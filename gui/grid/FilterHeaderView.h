#pragma once

#include <QHeaderView>
#include <QPoint>

namespace gui::grid {

    class FilterHeaderView : public QHeaderView {

    public:
        FilterHeaderView(Qt::Orientation orientation, QWidget* parent = 0);
        void mousePressEvent(QMouseEvent *event) override;
    };

} // namespace gui
