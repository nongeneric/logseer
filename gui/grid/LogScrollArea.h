#pragma once

#include "LogTableView.h"
#include <QAbstractScrollArea>

namespace gui::grid {

    class LogScrollArea : public QAbstractScrollArea {
        Q_OBJECT

        LogTableView* _view;
        int _rowCount = 0;

    public:
        explicit LogScrollArea(QWidget* parent = nullptr);
        void setWidget(LogTableView* view);
        void setRowCount(int count);
        void ensureVisible(int row);

    signals:

    public slots:

    protected:
        void resizeEvent(QResizeEvent *event) override;
        void scrollContentsBy(int dx, int dy) override;
    };

} // namespace seer::gui
