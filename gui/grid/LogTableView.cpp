#include "LogTableView.h"
#include "LogTable.h"
#include "seer/Log.h"
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QAction>
#include <QGuiApplication>
#include <QClipboard>

namespace gui::grid {

    void LogTableView::paintRow(QPainter* painter, int row, int y) {
        auto model = _table->model();
        auto columns = _table->header()->count();

        QFontMetrics fm(font());

        if (model->isSelected(row)) {
            QBrush b(palette().color(QPalette::Highlight));
            QRect r(0, y, width(), _rowHeight);
            painter->fillRect(r, b);
            painter->setPen(palette().color(QPalette::HighlightedText));
        } else {
            auto index = model->index(row, 0);
            assert(index.isValid());
            auto brush = model->data(index, Qt::ForegroundRole).value<QBrush>();
            painter->setPen(brush.color());
        }

        for (auto column = 0; column < columns; ++column) {
            auto x = _table->header()->sectionPosition(column);
            assert(row < model->rowCount({}));
            assert(column < model->columnCount({}));
            auto index = model->index(row, column);
            assert(index.isValid());
            auto text = model->data(index, Qt::DisplayRole).toString();
            auto sectionSize = _table->header()->sectionSize(column);
            if (!_table->expanded() && column == columns - 1) {
                sectionSize -= _table->scrollArea()->verticalScrollBar()->width();
                sectionSize -= _table->histMap()->width();
                sectionSize -= 2;
            }

            auto textWidth = [&](QString const& text, int len) {
                int index = 0;
                const auto& split = text.left(len).split("\t");
                for (int i = 0; i < split.size(); ++i) {
                    index += split[i].length();
                    if (i != split.size() - 1) {
                        index += _tabWidth - (index % _tabWidth);
                    }
                }
                return index * _charWidth;
            };

            auto isMessageColumn = column == columns - 1;
            if (_searcher && !model->isSelected(row) && (isMessageColumn || !_messageOnlyHighlight)) {
                int currentIndex = 0;
                for (;;) {
                    auto [first, len] = _searcher->search(text, currentIndex);
                    if (first == -1)
                        break;
                    int last = first + len;
                    auto sectionSize = _table->header()->sectionSize(column);
                    QRect r;
                    auto width = textWidth(text, first);
                    if (width >= sectionSize)
                        break;
                    r.setLeft(x + width);
                    r.setRight(x + std::min<float>(textWidth(text, last), sectionSize));
                    r.setTop(y);
                    r.setBottom(y + _rowHeight);
                    painter->fillRect(r, QBrush(QColor::fromRgb(0xfb, 0xfa, 0x08)));
                    currentIndex += len;
                }
            }

            QRect rect;
            rect.setLeft(x);
            rect.setRight(x + sectionSize);
            rect.setTop(y);
            rect.setBottom(y + _rowHeight);
            QTextOption option;
            option.setTabStopDistance(_charWidth * _tabWidth);
            option.setWrapMode(QTextOption::WrapAnywhere);
            painter->drawText(rect, text, option);
        }
    }

    int LogTableView::getRow(int y) {
        auto model = _table->model();
        if (!model)
            return -1;
        y = y + _table->scrollArea()->y() - _table->header()->height();
        return y / _rowHeight + _firstRow;
    }

    void LogTableView::copyToClipboard() {
        auto model = _table->model();
        if (!model)
            return;
        auto a = model->_index;(void)a;
        auto [first, last] = model->getSelection();
        if (first == -1)
            return;
        seer::log_infof("copying to clipboard lines [%d; %d)", first, last);
        QString text;
        for (auto i = first; i != last; ++i) {
            auto index = model->index(i, 0);
            text += model->data(index, (int)CellDataRole::RawLine).toString();
            if (i != last - 1) {
                text += "\n";
            }
        }
        QGuiApplication::clipboard()->setText(text);
    }

    LogTableView::LogTableView(QFont font, LogTable* parent) : QWidget(parent), _table(parent) {
        setFocusPolicy(Qt::ClickFocus);
        setFont(font);
        QFontMetricsF fm(font);
        _rowHeight = fm.height();
        _charWidth = fm.width(' ');
        setMouseTracking(true);

        auto copyAction = new QAction(_table->scrollArea());
        copyAction->setShortcuts(QKeySequence::Copy);
        copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(copyAction, &QAction::triggered, this, &LogTableView::copyToClipboard);
        addAction(copyAction);
    }

    void LogTableView::paintEvent(QPaintEvent* event) {
        auto model = _table->model();
        if (!model)
            return;

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing, QPainter::TextAntialiasing);

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
        if (row < _table->model()->rowCount({})) {
            _table->model()->setSelection(row);
        }
    }

    void LogTableView::mouseMoveEvent(QMouseEvent *event) {
        if (!(event->buttons() & Qt::LeftButton))
            return;
        auto row = getRow(event->y());
        if (row == -1)
            return;
        _table->model()->extendSelection(row);
        update();
    }

    void LogTableView::setFirstRow(int row) {
        _firstRow = row;
    }

    int LogTableView::visibleRows() {
        return height() / _rowHeight;
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
