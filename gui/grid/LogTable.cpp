#include "LogTable.h"

#include <QFontMetrics>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>

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
        auto lastColumnCharWidth =
            _model->maxColumnWidth(_model->columnCount({}) - 1);
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
            _header->setSectionResizeMode(last, QHeaderView::ResizeMode::Stretch);
        }
        emulateResize(this);
        emulateResize(_header);
    }

    LogTable::LogTable(QFont font, QWidget* parent) : QWidget(parent) {
        _header = new FilterHeaderView(this);
        _view = new LogTableView(font, this);
        _scrollArea = new LogScrollArea(this);
        _scrollArea->setWidget(_view, _header);
        _histMap = new HistMap(this);

        connect(_header, &QHeaderView::sectionResized, this, [=] {
            _view->update();
            emulateResize(this);
        });

        connect(_header, &QHeaderView::sectionClicked, this, [=](int column) {
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

        connect(_model, &QAbstractTableModel::modelReset, this, [=] {
            _scrollArea->setRowCount(_model->rowCount({}));
            _scrollArea->update();
        });

        connect(_model, &LogTableModel::selectionChanged, this, [=] {
            auto [first, last] = _model->getSelection();
            _scrollArea->ensureVisible(first);
            _view->update();
        });
    }

    void LogTable::setHist(const seer::Hist* hist) {
        _histMap->setHist(hist);
    }

    LogTableModel* LogTable::model() const {
        return _model;
    }

    FilterHeaderView* LogTable::header() const {
        return _header;
    }

    bool LogTable::eventFilter(QObject* watched, QEvent* event) {
        return QWidget::eventFilter(watched, event);
    }

    LogScrollArea* LogTable::scrollArea() const {
        return _scrollArea;
    }

    HistMap *LogTable::histMap() const {
        return _histMap;
    }

    bool LogTable::expanded() const {
        return _expanded;
    }

    void LogTable::showHistMap() {
        _showHistMap = true;
    }

    void LogTable::setSearchHighlight(std::string text, bool regex, bool caseSensitive) {
        _view->setSearchHighlight(text, regex, caseSensitive);
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
        _scrollArea->resize(size.width(),
                            size.height() - _header->height());

        auto vScrollBar = _scrollArea->verticalScrollBar();
        auto hScrollBar = _scrollArea->horizontalScrollBar();
        _histMap->resize(_showHistMap ? 28 : 0,
                         _scrollArea->height() - hScrollBar->height() -
                             _scrollArea->contentsMargins().top() -
                             _scrollArea->contentsMargins().bottom());
        _histMap->move(_scrollArea->width() - vScrollBar->width() -
                           _histMap->width() -
                           _scrollArea->contentsMargins().right(),
                       _header->height() + _scrollArea->contentsMargins().top());
        auto minHeaderWidth = _header->sectionPosition(_header->count() - 1);
        if (_expanded) {
            minHeaderWidth += _header->sectionSize(_header->count() - 1);
        }

        if (minHeaderWidth > size.width()) {
            hScrollBar->setMaximum(minHeaderWidth - size.width());
            hScrollBar->setPageStep(size.width());
        } else {
            hScrollBar->setMaximum(0);
            hScrollBar->setPageStep(0);
        }
    }

} // namespace gui::grid
