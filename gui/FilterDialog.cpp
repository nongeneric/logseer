#include "FilterDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

namespace gui {

    FilterDialog::FilterDialog(FilterTableModel* model, QWidget* parent)
        : QDialog(parent), _model(model)
    {
        auto vbox = new QVBoxLayout(this);
        auto hbox = new QHBoxLayout(this);
        _search = new QLineEdit(this);
        _selectAll = new QPushButton(this);
        _selectNone = new QPushButton(this);
        _selectFound = new QPushButton(this);
        _table = new QTableView(this);
        _table->setModel(model);

        hbox->addWidget(_selectAll);
        hbox->addWidget(_selectNone);
        hbox->addWidget(_selectFound);
        vbox->addWidget(_search);
        vbox->addLayout(hbox);
        vbox->addWidget(_table);

        setLayout(vbox);

        connect(_selectAll, &QPushButton::clicked, this, [=] {
            model->selectAll();
        });
        connect(_selectNone, &QPushButton::clicked, this, [=] {
            model->selectNone();
        });
        connect(_selectFound, &QPushButton::clicked, this, [=] {
            model->selectFound();
        });
        connect(_search, &QLineEdit::textEdited, this, [=](QString text) {
            model->search(text.toStdString());
        });
    }

} // namespace gui
