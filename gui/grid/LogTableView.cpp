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
            painter->setPen(palette().color(QPalette::ButtonText));
        }

        for (auto column = 0; column < columns; ++column) {
            auto x = _table->header()->sectionPosition(column);
            assert(row < model->rowCount({}));
            assert(column < model->columnCount({}));
            auto index = model->index(row, column);
            assert(index.isValid());
            auto text = model->data(index, Qt::DisplayRole).toString();
            auto sectionSize = _table->header()->sectionSize(column);
            if (column == columns - 1) {
                sectionSize -= _table->scrollArea()->verticalScrollBar()->width();
                if (!_table->expanded()) {
                    sectionSize -= _table->histMap()->width();
                    sectionSize -= 2;
                }
            }

            text = text.replace("\t", " ");
            auto elided = fm.elidedText(text, Qt::ElideRight, sectionSize);

            if (!_searchText.isEmpty() && !model->isSelected(row)) {
                int64_t index = 0;
                for (;;) {
                    auto caseSensitive = _searchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                    index = text.indexOf(_searchText, index, caseSensitive);
                    if (index == -1)
                        break;
                    int first = index;
                    int last = first + _searchText.size();
                    QRect r;
                    r.setLeft(x + fm.width(text, first));
                    r.setRight(x + fm.width(text, last));
                    r.setTop(y);
                    r.setBottom(y + _rowHeight);
                    painter->fillRect(r, QBrush(QColor::fromRgb(0xfb, 0xfa, 0x08)));
                    index += _searchText.size();
                }
            }

            painter->drawText(x, y, sectionSize, _rowHeight, 0, elided);
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

    LogTableView::LogTableView(LogTable* parent) : QWidget(parent), _table(parent) {
        setFocusPolicy(Qt::ClickFocus);
        setFont(QFont("Mono"));
        QFontMetricsF fm(font());
        _rowHeight = fm.height();
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

    void LogTableView::setSearchHighlight(std::string text, bool caseSensitive) {
        _searchText = QString::fromStdString(text);
        _searchCaseSensitive = caseSensitive;
        update();
    }

} // namespace gui::grid
