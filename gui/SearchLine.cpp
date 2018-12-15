#include "SearchLine.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>

namespace gui {

    SearchLine::SearchLine(QWidget* parent) : QWidget(parent) {
        auto edit = new QLineEdit();
        auto button = new QPushButton();
        button->setText("Search");

        auto topHbox = new QHBoxLayout();
        topHbox->addWidget(edit);
        topHbox->addWidget(button);

        auto caseSensitive = new QCheckBox();
        caseSensitive->setText("Case-sensitive");

        _status = new QLabel();
        _progress = new QProgressBar();
        _progress->setMaximum(100);
        _progress->hide();

        auto bottomHbox = new QHBoxLayout();
        bottomHbox->addWidget(_status);
        bottomHbox->addWidget(_progress);
        bottomHbox->addStretch();
        bottomHbox->addWidget(caseSensitive);

        auto vbox = new QVBoxLayout();
        vbox->addLayout(topHbox);
        vbox->addLayout(bottomHbox);
        vbox->setAlignment(caseSensitive, Qt::AlignRight);
        setLayout(vbox);

        connect(button,
                &QPushButton::clicked,
                this,
                [=] {
            emit searchRequested(edit->text().toStdString(), caseSensitive->isChecked());
        });
    }

    void SearchLine::setStatus(std::string status) {
        _status->setText(QString::fromStdString(status));
    }

    void SearchLine::setProgress(int progress) {
        if (progress == -1) {
            _progress->hide();
            return;
        }
        _progress->show();
        _progress->setValue(progress);
    }

} // namespace gui
