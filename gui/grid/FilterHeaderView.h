#pragma once

#include <QHeaderView>
#include <QPoint>

namespace gui::grid {

    class LogTable;

    class FilterHeaderView : public QHeaderView {
        LogTable* _table;

    public:
        FilterHeaderView(LogTable* table);
        void mousePressEvent(QMouseEvent *event) override;
        LogTable* logTable() const;
        QSize minimumSizeHint() const override;
    };

} // namespace gui
