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

    const int regularGrapheme = 0;
    const int selectedGrapheme = 1;
    const int highlightedGrapheme = 2;

    std::vector<int> graphemes;

    for (auto column = 0; column < columns; ++column) {
        auto x = _table->header()->sectionPosition(column);
        assert(row < model->rowCount({}));
        assert(column < model->columnCount({}));
        auto index = model->index(row, column);
        assert(index.isValid());

        auto text = model->data(index, Qt::DisplayRole).toString();
        GraphemeMap gmap(text);

        graphemes.clear();
        graphemes.resize(gmap.graphemeSize());

        if (isRowSelected) {
            ranges::fill(graphemes, selectedGrapheme);
        } else if (columnSelection && columnSelection->row == row &&
                   columnSelection->column == column) {
            assert(columnSelection->first >= 0);
            auto first = columnSelection->first;
            auto last = std::min(columnSelection->last, text.size() - 1);
            auto it = begin(graphemes);
            std::tie(first, last) = gmap.toVisibleRange(
                first, last, _selectWords ? VisibleRangeType::Word : VisibleRangeType::Grapheme);
            ranges::fill(it + first, it + last + 1, selectedGrapheme);
        }

        auto isMessageColumn = column == columns - 1;
        if (_searcher && (isMessageColumn || !_messageOnlyHighlight)) {
            int currentIndex = 0;
            for (;;) {
                auto [first, len] = _searcher->search(text, currentIndex);
                if (first == -1 || len == 0)
                    break;

                for (int i = first; i < first + len; ++i) {
                    auto [left, right] = gmap.indexToGraphemeRange(i);
                    while (left <= right) {
                        graphemes.at(left) |= highlightedGrapheme;
                        ++left;
                    }
                }
                currentIndex = first + len;
            }
        }

        foreachRange(graphemes, [&] (int start, int len) {
            QColor foreground;
            QBrush background;
            switch (graphemes[start]) {
            case regularGrapheme:
                foreground = palette().color(QPalette::Text);
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
            r.setLeft(x + start * _charWidth);
            r.setRight(x + (start + len) * _charWidth);
            r.setTop(y);
            r.setBottom(y + _rowHeight);
            painter->fillRect(r, background);

            QTextOption option;
            option.setWrapMode(QTextOption::NoWrap);
            painter->setPen(foreground);
            auto textWithoutTabs = gmap.graphemeRangeToString(start, start + len - 1);
            painter->drawText(r, textWithoutTabs, option);
        });
    }
}

int LogTableView::getRow(int y) {
    auto model = _table->model();
    if (!model)
        return -1;
    y = y + _table->scrollArea()->y() - _table->header()->height();
    return y / _rowHeight + _firstRow;
}

std::tuple<int, int> LogTableView::getColumn(int x) {
    auto columns = _table->header()->count();
    for (int c = 0; c < columns; ++c) {
        auto left = _table->header()->sectionPosition(c);
        auto right = c == columns - 1 ? std::numeric_limits<int>::max()
                                      : _table->header()->sectionPosition(c + 1);
        if (left <= x && x < right) {
            return {c, (x - left) / _charWidth};
        }
    }
    assert(false);
    return {0,0};
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
        auto index = model->index(columnSelection->row, columnSelection->column);
        auto text = model->data(index, Qt::DisplayRole).toString();
        GraphemeMap gmap(text);
        auto [left, right] =
            gmap.toVisibleRange(columnSelection->first,
                                columnSelection->last,
                                _selectWords ? VisibleRangeType::Word : VisibleRangeType::Grapheme);
        auto ileft = std::get<0>(gmap.graphemeToIndexRange(left));
        auto iright = std::get<1>(gmap.graphemeToIndexRange(right));
        QGuiApplication::clipboard()->setText(text.mid(ileft, iright - ileft + 1));
    }
}

LogTableView::LogTableView(QFont font, LogTable* parent) : QWidget(parent), _table(parent) {
    setFocusPolicy(Qt::ClickFocus);
    setFont(font);
    QFontMetricsF fm(font);
    _rowHeight = fm.height();
    _charWidth = fm.width(' ');
    setMouseTracking(true);

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
        if (!_table->model())
            return;

        auto selection = _table->model()->getSelection();
        if (std::holds_alternative<std::monostate>(selection))
            return;

        auto rowSelection = std::get_if<RowSelection>(&selection);

        QMenu menu(this);
        menu.addAction(copyAction);
        if (rowSelection) {
            menu.addAction(copyFormattedAction);
        }
        menu.exec(mapToGlobal(pos));
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
    auto row = getRow(event->y());
    if (row == -1)
        return;
    auto [column, index] = getColumn(event->x());
    if (row < _table->model()->rowCount({})) {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            _table->model()->extendSelection(row, column, index);
        } else {
            _table->model()->setSelection(row, column, index);
            _selectWords = false;
        }
    }
}

void LogTableView::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton))
        return;
    auto row = getRow(event->y());
    if (row == -1)
        return;
    auto x = event->x();
    if (x < 0)
        return;
    auto [column, index] = getColumn(x);
    _table->model()->extendSelection(row, column, index);
    update();
}

void LogTableView::mouseDoubleClickEvent(QMouseEvent*) {
    if (!_table->model())
        return;
    _table->model()->forceColumnSelection();
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
    return _charWidth;
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

} // namespace gui::grid
