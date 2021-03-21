#include "LogTable.h"

#include <QFontMetrics>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>

#include "LogTableView.h"

namespace gui::grid {

constexpr inline int g_columnHeaderPadding = 20;
constexpr inline int g_defaultMessageColumnSize = 1000;

void emulateResize(QWidget* widget) {
    auto size = widget->size();
    size.rheight()++;
    widget->resize(size);
    size.rheight()--;
    widget->resize(size);
}

void LogTable::setColumnWidth(int column, int longestColumnIndex) {
    QFontMetricsF fm(_header->font());
    auto isIndexed = _model->headerData(column, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool();
    auto headerText = _model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();
    auto headerWidth = fm.width(headerText);
    if (isIndexed) {
        headerWidth += _header->style()->pixelMetric(QStyle::PM_SmallIconSize);
    }
    float pxWidth = 0;
    auto autosize = _model->headerData(column, Qt::Horizontal, (int)HeaderDataRole::IsAutosize).toBool();
    if (autosize && longestColumnIndex != -1) {
        if (isIndexed) {
            for (const auto& value : _model->values(column)) {
                pxWidth = std::max(pxWidth, _view->textWidth(QString::fromStdString(value)));
            }
        } else if (autosize) {
            auto value = _model->data(_model->index(longestColumnIndex, column), Qt::DisplayRole).toString();
            pxWidth = _view->textWidth(value);
        }
    }
    pxWidth = std::clamp<int>(pxWidth, headerWidth, _header->maximumSectionSize());
    _header->resizeSection(column, pxWidth + g_columnHeaderPadding);
}

LogTable::LogTable(gui::LogFile* logFile, QFont font, QWidget* parent)
    : QWidget(parent), _logFile(logFile)
{
    _header = new FilterHeaderView(this);
    _view = new LogTableView(font, this);
    _scrollArea = new LogScrollArea(this);
    _scrollArea->setWidget(_view, _header);
    _histMap = new HistMap(this);

    connect(_header, &QHeaderView::sectionResized, this, [this] {
        _view->update();
        emulateResize(this);
    });

    connect(_header, &QHeaderView::sectionClicked, this, [this](int column) {
        if (column != 0) {
            emit requestFilter(column);
        }
    });
}

void LogTable::setModel(LogTableModel* model) {
    _model = model;
    _header->setModel(model);

    if (!model) {
        _view->update();
        return;
    }

    auto count = model->columnCount(QModelIndex());
    for (auto i = 0; i < count; ++i) {
        _header->setSectionResizeMode(i, QHeaderView::ResizeMode::Interactive);
        setColumnWidth(i, -1);
    }
    _header->setSectionResizeMode(count - 1, QHeaderView::ResizeMode::Fixed);
    setColumnWidth(0, model->rowCount({}) - 1);
    _header->resizeSection(count - 1, g_defaultMessageColumnSize);
    _view->invalidateCache();
    _view->update();
    _header->updateGeometry();
    _scrollArea->setRowCount(_model->rowCount({}));

    connect(_model, &QAbstractTableModel::modelReset, this, [this] {
        _scrollArea->setRowCount(_model->rowCount({}));
        _scrollArea->update();
        _view->invalidateCache();
    });

    connect(_model, &LogTableModel::selectionChanged, this, [this] {
        if (auto selection = _model->getRowSelection()) {
            if (!_scrollArea->isVisible(selection->first) &&
                !_scrollArea->isVisible(selection->last)) {
                _scrollArea->ensureVisible(selection->first);
            }
        }
        _view->update();
    });

    connect(_model, &LogTableModel::columnWidthsChanged, this, [this] {
        for (auto column = 0; column < _model->columnCount({}); ++column) {
            auto longestIndex = _model->headerData(column, Qt::Horizontal, (int)HeaderDataRole::LongestColumnIndex).toInt();
            setColumnWidth(column, longestIndex);
        }
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

LogFile* LogTable::logFile() const {
    return _logFile;
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

void LogTable::updateMessageWidth(int width) {
    width += g_columnHeaderPadding;
    auto messageColumn = _model->columnCount({}) - 1;
    if (_header->sectionSize(messageColumn) < width) {
        _header->resizeSection(messageColumn, width);
    }
}

void LogTable::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Home) {
        _scrollArea->ensureVisible(0);
    } else if (event->key() == Qt::Key_End) {
        auto max = std::max(0, _model->rowCount({}) - 1);
        _scrollArea->ensureVisible(max);
    } else {
        QWidget::keyPressEvent(event);
    }
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
