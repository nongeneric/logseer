#include "FilterHeaderView.h"

#include "LogTable.h"
#include "LogTableModel.h"
#include "Config.h"
#include <QMouseEvent>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOption>
#include <QStyleOptionHeader>
#include <QTableView>
#include <iostream>

namespace gui::grid {

class MyStyle : public QProxyStyle {
    FilterHeaderView* _parent;

public:
    MyStyle(FilterHeaderView* parent) : QProxyStyle(gui::g_qtStyleName), _parent(parent) {}

    void drawControl(ControlElement element,
                     const QStyleOption* opt,
                     QPainter* p,
                     const QWidget* widget) const override {
        if (element == QStyle::CE_HeaderLabel) {
            auto header = qstyleoption_cast<const QStyleOptionHeader*>(opt);
            QRect rect = header->rect;
            if (header->state & QStyle::State_On) {
                QFont fnt = p->font();
                fnt.setBold(true);
                p->setFont(fnt);
            }

            auto model = _parent->logTable()->model();

            if (!model)
                return;

            auto isIndexed = model->headerData(header->section,
                                               Qt::Orientation::Horizontal,
                                               (int)HeaderDataRole::IsIndexed).toBool();

            if (isIndexed) {
                auto isFilterActive =
                    model->headerData(header->section,
                                      Qt::Orientation::Horizontal,
                                      (int)HeaderDataRole::IsFilterActive).toBool();
                auto icon = QIcon(isFilterActive ? ":/filter-fill.svg" : ":/filter.svg");
                int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                auto pixmap = icon.pixmap(QSize(iconExtent, iconExtent), QIcon::Normal);
                auto y = rect.top() + (rect.height() - pixmap.height()) / 2;
                p->drawPixmap(rect.left(), y, pixmap);
                rect.setX(rect.x() + pixmap.width());
            }

            proxy()->drawItemText(p,
                                  rect,
                                  header->textAlignment,
                                  header->palette,
                                  (header->state & State_Enabled),
                                  header->text,
                                  QPalette::ButtonText);
        } else {
            QProxyStyle::drawControl(element, opt, p, widget);
        }
    }
};

FilterHeaderView::FilterHeaderView(LogTable* table)
    : QHeaderView(Qt::Horizontal, table), _table(table) {
    this->setStyle(new MyStyle(this));
    setSectionsClickable(true);
    setMouseTracking(true);
}

LogTable* FilterHeaderView::logTable() const {
    return _table;
}

} // namespace gui::grid
