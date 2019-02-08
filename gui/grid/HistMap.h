#pragma once

#include <QWidget>
#include "seer/Hist.h"

namespace gui {

    class HistMap : public QWidget {
        Q_OBJECT

        const seer::Hist* _hist = nullptr;

    public:
        HistMap(QWidget* parent = nullptr);
        void setHist(const seer::Hist *hist);

    protected:
        void paintEvent(QPaintEvent *event) override;
    };

} // namespace gui
