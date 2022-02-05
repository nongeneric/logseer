#pragma once

#include "FilterTableModel.h"
#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTableView>
#include <QDialog>
#include <memory>

namespace gui {

class FilterDialog : public QDialog {
    Q_OBJECT

    QLineEdit* _search;
    QPushButton* _selectAll;
    QPushButton* _selectNone;
    QPushButton* _selectFound;
    QTableView* _table;
    std::shared_ptr<FilterTableModel> _model;

public:
    explicit FilterDialog(std::shared_ptr<FilterTableModel> model, QWidget* parent = nullptr);

protected:
    bool event(QEvent* event) override;
};

} // namespace gui
