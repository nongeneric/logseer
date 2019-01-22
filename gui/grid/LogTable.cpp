#include "LogTable.h"

#include <QSizePolicy>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QFontMetrics>

#include "LogTableView.h"

namespace gui::grid {

    void emulateResize(QWidget* widget) {
        auto size = widget->size();
        size.rheight()++;
        widget->resize(size);
        size.rheight()--;
        widget->resize(size);
    }

    void LogTable::flipExpanded() {
        auto lastColumnCharWidth = _model->maxColumnWidth(_model->columnCount({}) - 1);
        if (lastColumnCharWidth == -1)
            return;
        _expanded = !_expanded;
        auto last = _header->count() - 1;
        if (_expanded) {
            _header->setSectionResizeMode(last,
                                          QHeaderView::ResizeMode::Interactive);
             QFontMetrics metrics(_view->font());
             auto lastColumnWidth =
                 metrics.size(Qt::TextSingleLine, "_") * lastColumnCharWidth;
             _header->resizeSection(last, lastColumnWidth.width());
        } else {
            _header->setSectionResizeMode(last,
                                          QHeaderView::ResizeMode::Stretch);
        }
        emulateResize(this);
        emulateResize(_header);
    }

    LogTable::LogTable(QWidget* parent) : QWidget(parent) {
        _header = new FilterHeaderView(this);
        _view = new LogTableView(this);
        _scrollArea = new LogScrollArea(this);
        _scrollArea->setWidget(_view, _header);

        connect(_header, &QHeaderView::sectionResized, this, [=] {
            _view->update();
            emulateResize(this);
        });

        connect(_header,
                &QHeaderView::sectionClicked,
                this,
                [=](int column) {
                    if (column == _header->count() - 1) {
                        flipExpanded();
                    } else if (column != 0) {
                        emit requestFilter(column);
                    }
                });
    }

    void LogTable::setModel(LogTableModel* model) {
        _model = model;
        _header->setModel(model);
        auto count = model->columnCount(QModelIndex());
        for (auto i = 0; i < count - 1; ++i) {
            _header->setSectionResizeMode(i, QHeaderView::ResizeMode::Interactive);
        }
        _header->setSectionResizeMode(count - 1, QHeaderView::ResizeMode::Stretch);
        _view->update();
        _header->updateGeometry();
        _scrollArea->setRowCount(_model->rowCount({}));

        connect(_model,
                &QAbstractTableModel::modelReset,
                this,
                [=] {
            _scrollArea->setRowCount(_model->rowCount({}));
            _scrollArea->update();
        });

        connect(_model,
                &LogTableModel::selectionChanged,
                this,
                [=] {
            auto [first, last] = _model->getSelection();
            _scrollArea->ensureVisible(first);
            _view->update();
        });
    }

    LogTableModel* LogTable::model() const {
        return _model;
    }

    FilterHeaderView *LogTable::header() const {
        return _header;
    }

    bool LogTable::eventFilter(QObject* watched, QEvent* event) {
        return QWidget::eventFilter(watched, event);
    }

    LogScrollArea* LogTable::scrollArea() const {
        return _scrollArea;
    }

    bool LogTable::expanded() const {
        return _expanded;
    }

    void LogTable::mousePressEvent(QMouseEvent* event) {
        QWidget::mousePressEvent(event);
    }

    void LogTable::keyPressEvent(QKeyEvent* event) {
        QWidget::keyPressEvent(event);
    }

    void LogTable::paintEvent(QPaintEvent* event) {
        QWidget::paintEvent(event);
    }

    void LogTable::resizeEvent(QResizeEvent*) {
        auto size = geometry();
        _header->resize(size.width() - _header->pos().x(), 26);
        _scrollArea->move(0, _header->height());
        _scrollArea->resize(size.width() - _scrollArea->pos().x(),
                            size.height() - _header->height());
        auto minHeaderWidth = _header->sectionPosition(_header->count() - 1);
        if (_expanded) {
            minHeaderWidth += _header->sectionSize(_header->count() - 1);
        }
        auto hScrollBar = _scrollArea->horizontalScrollBar();
        if (minHeaderWidth > size.width()) {
            hScrollBar->setMaximum(minHeaderWidth - size.width());
            hScrollBar->setPageStep(size.width());
        } else {
            hScrollBar->setMaximum(0);
            hScrollBar->setPageStep(0);
        }
    }

} // namespace gui::grid
