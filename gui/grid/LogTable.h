#pragma once

#include "../LogTableModel.h"
#include "FilterHeaderView.h"
#include "LogScrollArea.h"
#include <QWidget>

namespace gui::grid {

    class LogTableView;

    class LogTable : public QWidget {
        Q_OBJECT

        LogTableModel* _model = nullptr;
        FilterHeaderView* _header = nullptr;
        LogTableView* _view = nullptr;
        LogScrollArea* _scrollArea = nullptr;

    public:
        explicit LogTable(QWidget* parent = nullptr);
        void setModel(LogTableModel* model);
        LogTableModel* model() const;
        FilterHeaderView* header() const;

    signals:
        void requestFilter(int column);

    public slots:

    public:
        bool eventFilter(QObject *watched, QEvent *event) override;
        LogScrollArea* scrollArea() const;

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
    };

} // namespace gui::grid
