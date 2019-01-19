#include "LogScrollArea.h"
#include "LogTableModel.h"
#include <QResizeEvent>
#include <QScrollBar>

namespace gui::grid {

    LogScrollArea::LogScrollArea(QWidget* parent) : QAbstractScrollArea(parent) {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        setBackgroundRole(QPalette::Light);
        verticalScrollBar()->setPageStep(10);
    }

    void LogScrollArea::setWidget(LogTableView* view) {
        _view = view;
        view->setParent(viewport());
        view->installEventFilter(this);
        view->show();
    }

    void LogScrollArea::setRowCount(int count) {
        _rowCount = count;
        auto max = count - _view->visibleRows();
        if (max < 0)
            return;
        verticalScrollBar()->setMaximum(count - _view->visibleRows());
    }

    void LogScrollArea::ensureVisible(int row) {
        auto firstVisible = verticalScrollBar()->value();
        auto lastVisible = firstVisible + _view->visibleRows();
        if (firstVisible <= row && row <= lastVisible)
            return;
        auto newFirstRow = std::max(row - (lastVisible - firstVisible) / 2, 0);
        _view->setFirstRow(newFirstRow);
        verticalScrollBar()->setValue(newFirstRow);
    }

    void LogScrollArea::resizeEvent(QResizeEvent* event) {
        _view->resize(event->size());
        setRowCount(_rowCount);
    }

    void LogScrollArea::scrollContentsBy(int dx, int dy) {
        _view->setFirstRow(verticalScrollBar()->value());
        QAbstractScrollArea::scrollContentsBy(dx, dy);
    }

} // namespace gui::grid
