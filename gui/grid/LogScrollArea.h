#pragma once

#include "LogTableView.h"
#include "FilterHeaderView.h"
#include <QAbstractScrollArea>

namespace gui::grid {

    class LogScrollArea : public QAbstractScrollArea {
        Q_OBJECT

        LogTableView* _view;
        FilterHeaderView* _header;
        int _rowCount = 0;

    public:
        explicit LogScrollArea(QWidget* parent = nullptr);
        void setWidget(LogTableView* view, FilterHeaderView* header);
        void setRowCount(int count);
        void ensureVisible(int row);

    signals:

    public slots:

    protected:
        void resizeEvent(QResizeEvent *event) override;
        void scrollContentsBy(int dx, int dy) override;
    };

} // namespace seer::gui
