#include "FilterDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>

#include "seer/Log.h"

namespace gui {

FilterDialog::FilterDialog(QWidget* parent)
    : QDialog(parent)
{
    auto vbox = new QVBoxLayout(this);
    auto hbox = new QHBoxLayout(this);
    _search = new QLineEdit(this);
    _selectAll = new QPushButton(this);
    _selectNone = new QPushButton(this);
    _selectFound = new QPushButton(this);

    _table = new QTableView(this);
    _table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _table->setSelectionBehavior(QAbstractItemView::SelectRows);
    _table->installEventFilter(this);

    hbox->addWidget(_selectAll);
    hbox->addWidget(_selectNone);
    hbox->addWidget(_selectFound);
    vbox->addWidget(_search);
    vbox->addLayout(hbox);
    vbox->addWidget(_table);

    _selectAll->setText("&All");
    _selectNone->setText("&None");
    _selectFound->setText("&Found");

    setLayout(vbox);

    connect(_selectAll, &QPushButton::clicked, this, [this] {
        _model->selectAll();
    });
    connect(_selectNone, &QPushButton::clicked, this, [this] {
        _model->selectNone();
    });
    connect(_selectFound, &QPushButton::clicked, this, [this] {
        _model->selectFound();
    });
    connect(_search, &QLineEdit::textEdited, this, [this](QString text) {
        _model->search(text.toStdString());
    });
}

void FilterDialog::setModel(std::shared_ptr<FilterTableModel> model) {
    assert(model);
    if (_model) {
        _model->disconnect(this);
    }
    connect(model.get(), &FilterTableModel::searchReset, this, [this] {
        _search->clear();
    });
    _search->clear();
    _model = model;
    _model->search({});
    _table->setModel(model.get());
    if (_model) {
        _table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    }
}

bool FilterDialog::event(QEvent* event) {
    if (_model && event->type() == QEvent::KeyRelease && _table->hasFocus()) {
        auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            auto selectionModel = _table->selectionModel();
            std::vector<int> rows;
            for (auto row : selectionModel->selectedRows()) {
                rows.push_back(row.row());
            }
            _model->invertSelection(rows);
        }
    }
    return QDialog::event(event);
}

} // namespace gui
