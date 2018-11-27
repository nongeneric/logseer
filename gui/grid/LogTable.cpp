#include "LogTable.h"

#include <QSizePolicy>
#include <QVBoxLayout>
#include "LogTableView.h"

namespace gui::grid {

    LogTable::LogTable(QWidget* parent) : QWidget(parent) {
        _header = new FilterHeaderView(this);
        _header->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        _view = new LogTableView(this);
        _scrollArea = new QScrollArea(this);
        _scrollArea->setWidget(_view);
        _scrollArea->setBackgroundRole(QPalette::Light);
        _scrollArea->installEventFilter(_view);
        auto layout = new QVBoxLayout();
        layout->addWidget(_header);
        layout->addWidget(_scrollArea);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

        connect(_header, &QHeaderView::sectionResized, this, [=] {
            _view->update();
        });

        connect(_header,
                &QHeaderView::sectionClicked,
                this,
                [=](int column) {
                    if (column != 0) {
                        emit requestFilter(column - 1);
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

        connect(_model,
                &QAbstractTableModel::modelReset,
                this,
                [=] {
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

    QScrollArea* LogTable::scrollArea() const {
        return _scrollArea;
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

} // namespace gui::grid
