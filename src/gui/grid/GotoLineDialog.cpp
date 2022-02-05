#include "GotoLineDialog.h"
#include "seer/bformat.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

namespace gui {

GotoLineDialog::GotoLineDialog(QWidget* parent, int64_t lineCount)
    : QDialog(parent), _lineCount(lineCount)
{
    auto vbox = new QVBoxLayout(this);
    auto hbox = new QHBoxLayout(this);
    _line = new QLineEdit(this);
    _go = new QPushButton(this);
    _cancel = new QPushButton(this);

    hbox->addWidget(_go);
    hbox->addWidget(_cancel);
    vbox->addWidget(_line);
    vbox->addLayout(hbox);
    setWindowTitle("Goto...");

    _go->setText("&OK");
    _cancel->setText("&Cancel");
    _line->setPlaceholderText(bformat("1..%d", lineCount).c_str());

    setLayout(vbox);
    setFixedSize(sizeHint());

    connect(_line, &QLineEdit::textChanged, this, [this] {
        _go->setEnabled(getLine().has_value());
    });
    connect(_cancel, &QPushButton::clicked, this, [this] {
        reject();
    });
    connect(_go, &QPushButton::clicked, this, [this] {
        accept();
    });
}

std::optional<int64_t> GotoLineDialog::getLine() const {
    bool valid = false;
    auto value = _line->text().toLongLong(&valid);
    if (valid && 1 <= value && value <= _lineCount)
        return value;
    return {};
}

} // namespace gui
