#include "LogTableView.h"
#include "LogTable.h"
#include "seer/Index.h"
#include "seer/Log.h"
#include "gui/ForeachRange.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <cmath>
#include <range/v3/algorithm.hpp>

namespace gui::grid {

class GmapFontMetrics : public IFontMetrics {
    QFontMetricsF _fm;
    std::unordered_map<std::string, float> _cache;

public:
    GmapFontMetrics(QFontMetricsF fm) : _fm(fm) {}

    float width(const QString& str) override {
        // rely on short string optimization
        auto byteLen = str.size() * sizeof(QChar);
        auto ptr = reinterpret_cast<const char*>(str.data());
        std::string s(ptr, ptr + byteLen);

        auto it = _cache.find(s);
        if (it == end(_cache)) {
            it = _cache.emplace(s, _fm.horizontalAdvance(str)).first;
        }
        return it->second;
    }
};

void LogTableView::paintRow(QPainter* painter, int row, int y) {
    auto model = _table->model();
    auto columns = _table->header()->count();

    auto selection = model->getSelection();
    auto rowSelection = std::get_if<RowSelection>(&selection);
    auto columnSelection = std::get_if<ColumnSelection>(&selection);

    auto isRowSelected = rowSelection && rowSelection->isSelected(row);
    if (isRowSelected) {
        QBrush b(palette().color(QPalette::Highlight));
        QRect r(0, y, width(), _rowHeight);
        painter->fillRect(r, b);
    }

    std::unique_ptr<seer::ISearcher> selectionSearcher;
    if (columnSelection) {
        auto text = getSelectionText(*columnSelection);
        selectionSearcher = seer::createSearcher(text, false, false);
    }

    const int regularGrapheme = 0;
    const int selectedGrapheme = 1;
    const int highlightedGrapheme = 2;

    std::vector<int> graphemes;

    auto region = visibleRegion();
    QString segment;

    for (auto column = 0; column < columns; ++column) {
        auto x = _table->header()->sectionPosition(column);
        assert(row < model->rowCount({}));
        assert(column < model->columnCount({}));
        auto index = model->index(row, column);
        assert(index.isValid());

        auto gmap = getGraphemeMap(row, column);
        if (!gmap->graphemeSize())
            return;

        const auto& text = gmap->line();

        graphemes.clear();
        graphemes.resize(gmap->graphemeSize());

        if (isRowSelected) {
            ranges::fill(graphemes, selectedGrapheme);
        } else if (columnSelection && columnSelection->row == row &&
                   columnSelection->column == column) {
            assert(columnSelection->first >= 0);
            auto first = std::min(columnSelection->first, gmap->graphemeSize() - 1);
            auto last = std::min(columnSelection->last, gmap->graphemeSize() - 1);
            auto it = begin(graphemes);
            assert(first <= last);
            if (_selectWords) {
                std::tie(first, last) = gmap->extendToWordBoundary(first, last);
            }
            ranges::fill(it + first, it + last + 1, selectedGrapheme);
        }

        auto isMessageColumn = column == columns - 1;
        bool shouldApplySearcher =
            (_searcher && (isMessageColumn || !_messageOnlyHighlight)) || selectionSearcher;
        if (shouldApplySearcher) {
            auto searcher = selectionSearcher ? selectionSearcher.get() : _searcher.get();
            int currentIndex = 0;
            for (;;) {
                auto [first, len] = searcher->search(text, currentIndex);
                if (first == -1 || len == 0)
                    break;

                for (int i = first; i < first + len; ++i) {
                    auto grapheme = gmap->indexToGrapheme(i);
                    if (rowSelection || !(graphemes.at(grapheme) & selectedGrapheme)) {
                        graphemes.at(grapheme) |= highlightedGrapheme;
                    }
                }
                currentIndex = first + len;
            }
        }

        auto defaultForeground = _gmapCache.lookup({row, column})->foreground;

        foreachRange(graphemes, [&](int start, int len) {
            QColor foreground;
            QBrush background;
            switch (graphemes[start]) {
            case regularGrapheme:
                foreground = defaultForeground;
                background = palette().color(QPalette::Base);
                break;
            case selectedGrapheme:
                foreground = palette().color(QPalette::HighlightedText);
                background = palette().color(QPalette::Highlight);
                break;
            case highlightedGrapheme:
                foreground = palette().color(QPalette::WindowText);
                background = QColor::fromRgb(0xfb, 0xfa, 0x08);
                break;
            case selectedGrapheme | highlightedGrapheme:
                foreground = palette().color(QPalette::Text);
                background = palette().color(QPalette::Window);
                break;
            }

            QRectF r;
            r.setLeft(x + gmap->getPosition(start));
            r.setRight(x + gmap->getPosition(start + len));
            r.setTop(y);
            r.setBottom(y + _rowHeight);
            painter->fillRect(r, background);

            QTextOption option;
            option.setWrapMode(QTextOption::NoWrap);
            painter->setPen(foreground);

            for (int i = start; i < start + len; ++i) {
                r.setLeft(x + gmap->getPosition(i));
                r.setRight(x + gmap->getPosition(i + 1));

                if (!region.contains(r.toRect()))
                    continue;

                auto [ileft, iright] = gmap->graphemeToIndexRange(i);
                auto segmentLen = iright - ileft + 1;
                segment.resize(segmentLen);
                std::copy(text.begin() + ileft, text.begin() + iright + 1, segment.begin());
                painter->drawText(r, segment, option);
            }
        });
    }
}

LogicalPosition LogTableView::getLogicalPosition(int x, int y) const {
    auto model = _table->model();
    if (!model)
        return {};

    LogicalPosition position;
    y = y + _table->scrollArea()->y() - _table->header()->height();
    auto row = y / _rowHeight + _firstRow;
    if (row >= model->rowCount({}))
        return position;

    position.row = row;

    auto columns = _table->header()->count();
    for (int c = 0; c < columns; ++c) {
        auto left = _table->header()->sectionPosition(c);
        auto right = c == columns - 1 ? std::numeric_limits<int>::max()
                                      : _table->header()->sectionPosition(c + 1);
        if (left <= x && x < right) {
            auto text = model->data(model->index(row, c), Qt::DisplayRole).toString();
            GraphemeMap gmap(text, _gmapFontMetrics.get());
            position.column = c;
            position.grapheme = gmap.findGrapheme(x - left);
            return position;
        }
    }

    return position;
}

void LogTableView::copyToClipboard(bool raw) {
    auto model = _table->model();
    if (!model)
        return;

    auto selection = _table->model()->getSelection();
    auto rowSelection = std::get_if<RowSelection>(&selection);
    auto columnSelection = std::get_if<ColumnSelection>(&selection);

    if (rowSelection) {
        auto first = rowSelection->first;
        auto last = rowSelection->last;
        seer::log_infof("copying to clipboard lines [%d; %d)", first, last);
        QString text;
        auto append =  [&] (auto& line) {
            text += QString::fromStdString(line);
            text += "\n";
        };
        if (raw) {
            model->copyRawLines(first, last + 1, append);
        } else {
            model->copyLines(first, last + 1, append);
        }
        assert(text.size());
        text.resize(text.size() - 1);
        QGuiApplication::clipboard()->setText(text);
    } else if (columnSelection) {
        auto text = getSelectionText(*columnSelection);
        QGuiApplication::clipboard()->setText(text);
    }
}

void LogTableView::addColumnExcludeActions(int column, int row, QMenu& menu) {
    auto logFile = _table->logFile();
    if (!logFile)
        return;

    auto model = _table->model();
    auto isIndexed = model->headerData(column, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool();
    if (!isIndexed)
        return;

    auto columnName = model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();
    auto columnValue = model->data(model->index(row, column), Qt::DisplayRole).toString();

    auto excludeCaption = QString("Exclude: %0").arg(columnValue);
    auto includeCaption = QString("Include only: %0").arg(columnValue);
    auto clearFilterCaption = QString("Clear filter: %0").arg(columnName);

    auto excludeAction = new QAction(excludeCaption);
    connect(excludeAction, &QAction::triggered, [=] {
        logFile->excludeValue(column, columnValue.toStdString());
    });

    auto includeAction = new QAction(includeCaption);
    connect(includeAction, &QAction::triggered, [=] {
        logFile->includeOnlyValue(column, columnValue.toStdString());
    });

    auto clearFilterAction = new QAction(clearFilterCaption);
    connect(clearFilterAction, &QAction::triggered, [=] {
        logFile->clearFilter(column);
    });

    menu.addSeparator();
    menu.addAction(excludeAction);
    menu.addAction(includeAction);
    menu.addAction(clearFilterAction);
}

void LogTableView::addClearAllFiltersAction(QMenu& menu) {
    auto logFile = _table->logFile();
    if (!logFile)
        return;

    auto clearFiltersAction = new QAction("Clear all filters");
    connect(clearFiltersAction, &QAction::triggered, [=] {
        logFile->clearFilters();
    });

    menu.addSeparator();
    menu.addAction(clearFiltersAction);
}

std::shared_ptr<GraphemeMap> LogTableView::getGraphemeMap(int row, int column) {
    auto model = _table->model();
    assert(model);
    auto index = model->index(row, column);
    assert(index.isValid());
    if (auto entry = _gmapCache.lookup({row, column}))
        return entry->gmap;
    auto text = model->data(index, Qt::DisplayRole).toString();
    auto foreground = model->data(index, Qt::ForegroundRole).value<QColor>();
    auto gmap = std::make_shared<GraphemeMap>(text, _gmapFontMetrics.get());
    _gmapCache.insert({row, column}, {gmap, foreground});
    return gmap;
}

LogTableView::LogTableView(QFont font, LogTable* parent)
    : QWidget(parent), _table(parent), _gmapCache(200)
{
    setFocusPolicy(Qt::ClickFocus);
    setFont(font);
    QFontMetricsF fm(font);
    _rowHeight = fm.height();
    setMouseTracking(true);

    _gmapFontMetrics = std::make_unique<GmapFontMetrics>(fm);

    auto copyAction = new QAction("Copy selected", _table->scrollArea());
    copyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyAction, &QAction::triggered, [this] { LogTableView::copyToClipboard(true); });
    addAction(copyAction);

    auto copyFormattedAction = new QAction("Copy selected (formatted)", _table->scrollArea());
    copyFormattedAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    copyFormattedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyFormattedAction, &QAction::triggered, [this] { LogTableView::copyToClipboard(false); });
    addAction(copyFormattedAction);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, [=] (const QPoint& pos) {
        auto model = _table->model();
        if (!model)
            return;

        QMenu menu(this);

        auto position = getLogicalPosition(pos.x(), pos.y());
        auto columnCount = model->columnCount({});
        if (position.row != -1 && 1 <= position.column && position.column < columnCount - 1) {
            addColumnExcludeActions(position.column, position.row, menu);
        }

        menu.addSeparator();

        auto selection = model->getSelection();
        if (!std::holds_alternative<std::monostate>(selection)) {
            auto rowSelection = std::get_if<RowSelection>(&selection);
            menu.addAction(copyAction);
            if (rowSelection) {
                menu.addAction(copyFormattedAction);
            }
        }

        addClearAllFiltersAction(menu);

        if (!menu.actions().empty()) {
            menu.exec(mapToGlobal(pos));
        }
    });
}

void LogTableView::paintEvent(QPaintEvent* event) {
    auto model = _table->model();
    if (!model)
        return;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    int row = _firstRow;
    int y = 0;

    int maxY = event->rect().height() + _rowHeight;
    for (; row < _table->model()->rowCount(QModelIndex()); ++row) {
        painter.save();
        paintRow(&painter, row, y);
        painter.restore();
        y += _rowHeight;
        if (y > maxY)
            break;
    }
}

void LogTableView::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;
    auto position = getLogicalPosition(event->x(), event->y());
    if (position.row != -1 && position.row < _table->model()->rowCount({})) {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            _table->model()->extendSelection(position.row, position.column, position.grapheme);
        } else {
            _table->model()->setSelection(position.row, position.column, position.grapheme);
            _selectWords = false;
        }
    }
}

void LogTableView::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton))
        return;
    auto position = getLogicalPosition(event->x(), event->y());
    if (position.column == -1)
        return;
    if (_table->model()->extendSelection(position.row, position.column, position.grapheme)) {
        update();
    }
}

void LogTableView::mouseDoubleClickEvent(QMouseEvent* event) {
    auto p = getLogicalPosition(event->x(), event->y());
    if (p.row == -1)
        return;
    auto model = _table->model();
    assert(model);
    if (!model->data(model->index(p.row, p.column), Qt::DisplayRole).toString().size())
        return;
    model->forceColumnSelection();
    _selectWords = true;
    update();
}

void LogTableView::setFirstRow(int row) {
    _firstRow = row;
}

int LogTableView::visibleRows() {
    return height() / _rowHeight;
}

float LogTableView::charWidth() const {
    QFontMetrics fm(font());
    return fm.horizontalAdvance(" ");
}

void LogTableView::setSearchHighlight(std::string text,
                                      bool regex,
                                      bool caseSensitive,
                                      bool messageOnly) {
    _messageOnlyHighlight = messageOnly;
    auto qText = QString::fromStdString(text);
    _searcher = seer::createSearcher(qText, regex, caseSensitive);
    update();
}

QString LogTableView::getSelectionText(const ColumnSelection& columnSelection) {
    auto gmap = getGraphemeMap(columnSelection.row, columnSelection.column);

    auto left = columnSelection.first;
    auto right = columnSelection.last;

    if (_selectWords) {
        std::tie(left, right) = gmap->extendToWordBoundary(left, right);
    }

    if (left == -1 || right == -1)
        return "";

    left = std::min(left, gmap->graphemeSize() - 1);
    right = std::min(right, gmap->graphemeSize() - 1);

    auto ileft = std::get<0>(gmap->graphemeToIndexRange(left));
    auto iright = std::get<1>(gmap->graphemeToIndexRange(right));
    return gmap->line().mid(ileft, iright - ileft + 1);
}

} // namespace gui::grid
