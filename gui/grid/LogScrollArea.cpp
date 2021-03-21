#include "LogScrollArea.h"
#include "LogTableModel.h"
#include <QResizeEvent>
#include <QScrollBar>

namespace gui::grid {

std::tuple<int, int> LogScrollArea::visibleRowRange() {
    auto firstVisible = verticalScrollBar()->value();
    auto lastVisible = firstVisible + _view->visibleRows();
    return {firstVisible, lastVisible};
}

LogScrollArea::LogScrollArea(QWidget* parent) : QAbstractScrollArea(parent) {
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setBackgroundRole(QPalette::Light);
}

void LogScrollArea::setWidget(LogTableView* view, FilterHeaderView* header) {
    _view = view;
    _header = header;
    view->setParent(viewport());
    view->installEventFilter(this);
    view->show();
}

void LogScrollArea::setRowCount(int count) {
    _rowCount = count;
    auto max = std::max(count - _view->visibleRows(), 0);
    verticalScrollBar()->setMaximum(max);
}

void LogScrollArea::ensureVisible(int row) {
    if (isVisible(row))
        return;
    auto [first, last] = visibleRowRange();
    auto newFirstRow = std::max(row - (last - first) / 2, 0);
    _view->setFirstRow(newFirstRow);
    verticalScrollBar()->setValue(newFirstRow);
}

bool LogScrollArea::isVisible(int row) {
    auto [first, last] = visibleRowRange();
    return first <= row && row <= last;
}

void LogScrollArea::resizeEvent(QResizeEvent* event) {
    _view->resize(event->size());
    scrollContentsBy(0, 0);
    verticalScrollBar()->setPageStep(_view->visibleRows());
    setRowCount(_rowCount);
}

void LogScrollArea::scrollContentsBy(int dx, int dy) {
    _view->setFirstRow(verticalScrollBar()->value());
    _view->move(-horizontalScrollBar()->value(), 0);
    _view->resize(_view->size().width() - _view->pos().x(),
                  _view->size().height());
    _header->move(-horizontalScrollBar()->value(), 0);
    _header->resize(_header->size().width() - _header->pos().x(),
                    _header->size().height());
    QAbstractScrollArea::scrollContentsBy(dx, dy);
}

} // namespace gui::grid
