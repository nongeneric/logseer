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
        if (row == _selectedRow) {
            painter->drawRect(0, y, width(), _rowHeight);
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
            painter->drawText(x, y, sectionSize, 40, 0, elided);
        }
    }

    LogTableView::LogTableView(LogTable* parent) : QWidget(parent), _table(parent) {
        setFont(QFont("Mono"));
        QFontMetricsF fm(font());
        _rowHeight = fm.height();
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
            paintRow(&painter, row, y);
            y += _rowHeight;
            if (y > maxY)
                break;
        }
    }

    void LogTableView::mousePressEvent(QMouseEvent *event) {
        auto model = _table->model();
        if (!model)
            return;
        auto y = event->y() + _table->scrollArea()->y() - _table->header()->height();
        auto row = y / _rowHeight;
        _selectedRow = row < model->rowCount({}) ? row : -1;
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

} // namespace gui::grid
