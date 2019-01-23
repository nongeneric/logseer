#include "SearchLine.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QShortcut>

namespace gui {

    SearchLine::SearchLine(QWidget* parent) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto edit = new QLineEdit();
        _button = new QPushButton();
        _button->setText("Search");

        auto topHbox = new QHBoxLayout();
        topHbox->addWidget(edit);
        topHbox->addWidget(_button);

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

        auto focusShortcut = new QShortcut(QKeySequence::Find, this);

        connect(focusShortcut, &QShortcut::activated, this, [=] {
            edit->selectAll();
            edit->setFocus();
        });

        auto search = [=] {
            emit searchRequested(edit->text().toStdString(), caseSensitive->isChecked());
        };

        connect(edit, &QLineEdit::returnPressed, this, search);
        connect(_button, &QPushButton::clicked, this, search);
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

    void SearchLine::setSearchEnabled(bool enabled) {
        _button->setEnabled(enabled);
    }

} // namespace gui
