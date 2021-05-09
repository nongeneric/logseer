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

namespace gui::grid {

static constexpr int g_searchRangeExtraGraphemes = 150;
static constexpr int g_gmapCacheSize = 1000;
static constexpr int g_sectionPadding = 5;

struct GraphemeRange {
    int first = -1;
    int last = -1;

    int size() const {
        return last - first + 1;
    }
};

GraphemeRange getGraphemeRange(const GraphemeMap* gmap, const QRect& rect, int extraGraphemes) {
    auto first = gmap->findGrapheme(rect.left()) - extraGraphemes;
    auto last = gmap->findGrapheme(rect.right()) + extraGraphemes;
    first = std::clamp(first, 0, gmap->graphemeSize() - 1);
    last = std::clamp(last, 0, gmap->graphemeSize() - 1);
    assert(first <= last);
    return {first, last};
}

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
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
            it = _cache.emplace(s, _fm.width(str)).first;
#else
            it = _cache.emplace(s, _fm.horizontalAdvance(str)).first;
#endif
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
        auto sectionSize = _table->header()->sectionSize(column) - g_sectionPadding;
        assert(row < model->rowCount({}));
        assert(column < model->columnCount({}));
        assert(model->index(row, column).isValid());

        auto gmap = getGraphemeMap(row, column);
        if (!gmap->graphemeSize())
            continue;

        _table->updateMessageWidth(gmap->pixelWidth());

        const auto& text = gmap->line();

        graphemes.clear();

        auto rect = region.boundingRect();
        rect.adjust(-x, 0, -x, 0);
        auto searchRange = getGraphemeRange(gmap.get(), rect, g_searchRangeExtraGraphemes);
        graphemes.resize(searchRange.size());

        if (isRowSelected) {
            std::fill(begin(graphemes), end(graphemes), selectedGrapheme);
        } else if (columnSelection && columnSelection->row == row &&
                   columnSelection->column == column) {
            assert(columnSelection->first >= 0);
            auto first = std::min(columnSelection->first, gmap->graphemeSize() - 1);
            auto last = std::min(columnSelection->last, gmap->graphemeSize() - 1);
            assert(first <= last);
            if (_selectWords) {
                std::tie(first, last) = gmap->extendToWordBoundary(first, last);
            }
            first = std::clamp(first, searchRange.first, searchRange.last) - searchRange.first;
            last = std::clamp(last, searchRange.first, searchRange.last) - searchRange.first;
            for (auto i = first; i <= last; ++i) {
                graphemes.at(i) = selectedGrapheme;
            }
        }

        auto isMessageColumn = column == columns - 1;
        bool shouldApplySearcher =
            (_searcher && (isMessageColumn || !_messageOnlyHighlight)) || selectionSearcher;
        if (shouldApplySearcher) {
            auto searcher = selectionSearcher ? selectionSearcher.get() : _searcher.get();
            int currentIndex = std::get<0>(gmap->graphemeToIndexRange(searchRange.first));
            while (currentIndex < searchRange.last) {
                auto [first, len] = searcher->search(text, currentIndex);
                if (first == -1 || len == 0)
                    break;

                auto last = std::min(first + len - 1, searchRange.last);

                for (int i = first; i <= last; ++i) {
                    auto grapheme = gmap->indexToGrapheme(i) - searchRange.first;
                    if (rowSelection || !(graphemes.at(grapheme) & selectedGrapheme)) {
                        graphemes.at(grapheme) |= highlightedGrapheme;
                    }
                }
                currentIndex += len;
            }
        }

        auto defaultForeground = _gmapCache.lookup({row, column})->foreground;

        foreachRange(graphemes, [&](int start, int len) {
            QColor foreground;
            QBrush background;
            switch (graphemes.at(start)) {
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

            start += searchRange.first;

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

                if (r.left() >= x + sectionSize)
                    break;

                r.setRight(std::min<double>(r.right(), x + sectionSize));

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

LogicalPosition LogTableView::getLogicalPosition(int x, int y) {
    auto model = _table->model();
    if (!model)
        return {};

    LogicalPosition position;
    y = y + _table->scrollArea()->y() - _table->header()->height();
    auto row = y / _rowHeight + _firstRow;
    if (row < 0 || row >= model->rowCount({}))
        return position;

    position.row = row;

    auto columns = _table->header()->count();
    for (int c = 0; c < columns; ++c) {
        auto left = _table->header()->sectionPosition(c);
        auto right = c == columns - 1 ? std::numeric_limits<int>::max()
                                      : _table->header()->sectionPosition(c + 1);
        if (left <= x && x < right) {
            auto gmap = getGraphemeMap(row, c);
            position.column = c;
            position.grapheme = gmap->findGrapheme(x - left);
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
    : QWidget(parent), _table(parent), _gmapCache(g_gmapCacheSize)
{
    setFocusPolicy(Qt::ClickFocus);
    setFont(font);
    QFontMetricsF fm(font);
    _rowHeight = fm.height();
    setMouseTracking(true);

    _gmapFontMetrics = std::make_unique<GmapFontMetrics>(fm);

    auto copyAction = new QAction(_table->scrollArea());
    copyAction->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | Qt::Key_C));
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyAction, &QAction::triggered, [this] { LogTableView::copyToClipboard(true); });
    addAction(copyAction);

    auto copyFormattedAction = new QAction(_table->scrollArea());
    copyFormattedAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    copyFormattedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyFormattedAction, &QAction::triggered, [this] { LogTableView::copyToClipboard(false); });
    addAction(copyFormattedAction);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, [=, this] (const QPoint& pos) {
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
            auto columnSelection = std::get_if<ColumnSelection>(&selection);
            if (columnSelection && _selectWords) {
                auto gmap = getGraphemeMap(columnSelection->row, columnSelection->column);
                std::tie(columnSelection->first, columnSelection->last) =
                    gmap->extendToWordBoundary(columnSelection->first, columnSelection->last);
            }
            auto number = rowSelection ? bformat("%d lines", rowSelection->size())
                                       : bformat("%d characters", columnSelection->size());
            copyAction->setText(QString::fromStdString(bformat("Copy %s", number)));
            menu.addAction(copyAction);
            if (rowSelection) {
                copyFormattedAction->setText(
                    QString::fromStdString(bformat("Copy %s (with headers)", number)));
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

    _gmapCache.setCapacity(g_gmapCacheSize * model->columnCount({}));

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

float LogTableView::textWidth(const QString& text) const {
    return _gmapFontMetrics->width(text);
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

void LogTableView::invalidateCache() {
    _gmapCache.clear();
}

} // namespace gui::grid
