#include "SearchLine.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>

namespace gui {

    SearchLine::SearchLine(QWidget* parent) : QWidget(parent) {
        auto edit = new QLineEdit();
        auto button = new QPushButton();
        button->setText("Search");
        auto layout = new QHBoxLayout();
        layout->addWidget(edit);
        layout->addWidget(button);
        setLayout(layout);

        connect(button,
                &QPushButton::clicked,
                this,
                [=] {
            emit requestSearch(edit->text().toStdString());
        });
    }

} // namespace gui
