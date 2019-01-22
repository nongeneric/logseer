#include "LogTableView.h"
#include "LogTable.h"
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>

namespace gui::grid {

    void LogTableView::paintRow(QPainter* painter, int row, int y) {
        auto model = _table->model();
        auto columns = _table->header()->count();

        if (model->isSelected(row)) {
            QBrush b(palette().color(QPalette::Highlight));
            QRect r(0, y, width(), _rowHeight);
            painter->fillRect(r, b);
            painter->setPen(palette().color(QPalette::HighlightedText));
        } else {
            painter->setPen(palette().color(QPalette::ButtonText));
        }

        QFontMetrics metrics(font());
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
            }
            auto elided = metrics.elidedText(text, Qt::ElideRight, sectionSize);
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

    LogTableView::LogTableView(LogTable* parent) : QWidget(parent), _table(parent) {
        setFont(QFont("Mono"));
        QFontMetricsF fm(font());
        _rowHeight = fm.height();
        setMouseTracking(true);
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

} // namespace gui::grid
