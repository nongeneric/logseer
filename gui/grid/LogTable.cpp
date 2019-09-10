#include "LogTable.h"

#include <QFontMetrics>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>

#include "LogTableView.h"

namespace gui::grid {

    constexpr inline int g_columnAutosizePadding = 3;
    constexpr inline int g_defaultMessageColumnSize = 1000;

    void emulateResize(QWidget* widget) {
        auto size = widget->size();
        size.rheight()++;
        widget->resize(size);
        size.rheight()--;
        widget->resize(size);
    }

    void LogTable::setColumnWidth(int column, int width) {
        QFontMetricsF fm(_header->font());
        auto isIndexed = _model->headerData(column, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool();
        auto headerText = _model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();
        const auto padding = 20;
        auto headerWidth = fm.width(headerText) + padding;
        if (isIndexed) {
            headerWidth += _header->style()->pixelMetric(QStyle::PM_SmallIconSize);
        }
        auto pxWidth = static_cast<int>(_view->charWidth() * width);
        pxWidth = std::max(static_cast<int>(headerWidth), pxWidth);
        pxWidth = std::min(pxWidth, _header->maximumSectionSize());
        _header->resizeSection(column, pxWidth);
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
            if (column != 0) {
                emit requestFilter(column);
            }
        });
    }

    void LogTable::setModel(LogTableModel* model) {
        _model = model;
        _header->setModel(model);

        if (!model)
            return;

        auto count = model->columnCount(QModelIndex());
        for (auto i = 0; i < count; ++i) {
            _header->setSectionResizeMode(i, QHeaderView::ResizeMode::Interactive);
        }
        _header->setSectionResizeMode(count - 1, QHeaderView::ResizeMode::Fixed);
        setColumnWidth(count - 1, g_defaultMessageColumnSize);
        _view->update();
        _header->updateGeometry();
        _scrollArea->setRowCount(_model->rowCount({}));

        connect(_model, &QAbstractTableModel::modelReset, this, [=] {
            _scrollArea->setRowCount(_model->rowCount({}));
            _scrollArea->update();
        });

        connect(_model, &LogTableModel::selectionChanged, this, [=] {
            auto [first, last] = _model->getSelection();
            if (!_scrollArea->isVisible(first) &&
                !_scrollArea->isVisible(last)) {
                _scrollArea->ensureVisible(first);
            }
            _view->update();
        });

        connect(_model, &LogTableModel::columnWidthsChanged, this, [=] {
            for (auto column = 0; column < _model->columnCount({}); ++column) {
                auto autosize = _model->headerData(column, Qt::Horizontal, (int)HeaderDataRole::Autosize).toInt();
                if (autosize != -1) {
                    setColumnWidth(column, autosize + g_columnAutosizePadding);
                }
            }
            auto lastColumn = _model->columnCount({}) - 1;
            setColumnWidth(lastColumn, _model->maxColumnWidth(lastColumn));
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

    void LogTable::showHistMap() {
        _showHistMap = true;
    }

    void LogTable::setSearchHighlight(std::string text,
                                      bool regex,
                                      bool caseSensitive,
                                      bool messageOnly) {
        _view->setSearchHighlight(text, regex, caseSensitive, messageOnly);
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

        auto histMapWidth = _showHistMap ? 28 : 0;
        auto vScrollBar = _scrollArea->verticalScrollBar();
        auto hScrollBar = _scrollArea->horizontalScrollBar();
        _histMap->resize(histMapWidth,
                         _scrollArea->height() - hScrollBar->height() -
                             _scrollArea->contentsMargins().top() -
                             _scrollArea->contentsMargins().bottom());
        _histMap->move(_scrollArea->width() - vScrollBar->width() -
                           _histMap->width() -
                           _scrollArea->contentsMargins().right(),
                       _header->height() + _scrollArea->contentsMargins().top());

        auto contentWidth = _header->sectionPosition(_header->count() - 1) +
                            _header->sectionSize(_header->count() - 1);

        auto availableWidth = size.width() - histMapWidth - vScrollBar->width();
        if (contentWidth > availableWidth) {
            hScrollBar->setMaximum(contentWidth - availableWidth);
            hScrollBar->setPageStep(size.width());
        } else {
            hScrollBar->setMaximum(0);
            hScrollBar->setPageStep(0);
        }
    }

} // namespace gui::grid
