#include "SearchLine.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpacerItem>

namespace gui {

    SearchLine::SearchLine(QWidget* parent) : QWidget(parent) {
        auto edit = new QLineEdit();
        auto button = new QPushButton();
        button->setText("Search");

        auto hbox = new QHBoxLayout();
        hbox->addWidget(edit);
        hbox->addWidget(button);

        auto caseSensitive = new QCheckBox();
        caseSensitive->setText("Case-sensitive");

        auto vbox = new QVBoxLayout();
        vbox->addLayout(hbox);
        vbox->addWidget(caseSensitive);
        vbox->setAlignment(caseSensitive, Qt::AlignRight);
        setLayout(vbox);

        connect(button,
                &QPushButton::clicked,
                this,
                [=] {
            emit requestSearch(edit->text().toStdString(), caseSensitive->isChecked());
        });
    }

} // namespace gui
