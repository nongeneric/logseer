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
            auto elided = metrics.elidedText(text, Qt::ElideRight, sectionSize);
            painter->drawText(x, y, sectionSize, _rowHeight, 0, elided);
        }
    }

    int LogTableView::getRow(int y) {
        auto model = _table->model();
        if (!model)
            return -1;
        y = y + _table->scrollArea()->y() - _table->header()->height();
        return y / _rowHeight;
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

        int minY = std::max(0, event->rect().y() - _rowHeight);
        int maxY = minY + event->rect().height() + _rowHeight;

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing, QPainter::TextAntialiasing);

        int row = minY / _rowHeight;
        int y = row * _rowHeight;

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
        _table->model()->setSelection(row < _table->model()->rowCount({}) ? row : -1);
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

    QSize LogTableView::sizeHint() const {
        auto model = _table->model();
        auto rows = model ? model->rowCount(QModelIndex()) : 1;
        return {1000, rows * _rowHeight};
    }

    bool LogTableView::eventFilter(QObject* watched, QEvent* event) {
        if (auto scrollArea = _table->scrollArea()) {
            if (watched == scrollArea && event->type() == QEvent::Resize) {
                auto resizeEvent = static_cast<QResizeEvent*>(event);
                auto size = resizeEvent->size();
                size.setHeight(sizeHint().height());
                int width = size.width() -
                            scrollArea->verticalScrollBar()->sizeHint().width();
                size.setWidth(width);
                resize(size);
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    int LogTableView::getRowY(int row) {
        return _rowHeight * row;
    }

} // namespace gui::grid
