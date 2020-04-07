#include "HistMap.h"

#include <QPainter>
#include <QPaintEvent>

namespace gui {

HistMap::HistMap(QWidget* parent) : QWidget(parent) {}

void HistMap::setHist(const seer::Hist* hist) {
    _hist = hist;
    repaint();
}

void HistMap::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    QBrush b(palette().color(QPalette::Light));
    auto r = event->rect();
    painter.fillRect(r, b);
    painter.drawLine(r.left(), r.top(), r.left(), r.bottom());

    if (!_hist)
        return;

    auto height = r.height();
    auto width = r.width() - 4;
    for (auto i = 0; i < height; ++i) {
        if (_hist->get(i, height)) {
            painter.drawLine(3, i, width, i);
        }
    }
}

} // namespace gui
