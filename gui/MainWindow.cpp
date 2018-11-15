#include "MainWindow.h"

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "LogFile.h"
#include "grid/FilterHeaderView.h"
#include <fstream>

namespace gui {

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        _tabWidget = new QTabWidget(this);
    }

    void MainWindow::openLog(std::string path) {
        auto file = new LogFile(path);
        file->parse();

        auto table = new QTableView(this);
        table->setModel(file->logTableModel());
        table->setHorizontalHeader(new grid::FilterHeaderView(Qt::Horizontal, table));

        auto count = file->logTableModel()->columnCount(QModelIndex());
        for (auto i = 0; i < count - 1; ++i) {
            table->horizontalHeader()->setSectionResizeMode(
                i, QHeaderView::ResizeMode::Interactive);
        }
        table->horizontalHeader()->setSectionResizeMode(
            count - 1, QHeaderView::ResizeMode::Stretch);
        table->setSelectionMode(QAbstractItemView::SelectionMode::ContiguousSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        table->verticalHeader()->hide();
        table->setShowGrid(false);
        table->setWordWrap(false);
        setCentralWidget(table);

        connect(file, &LogFile::filterRequested, this, [=](auto filterModel, int column) {
            connect(filterModel, &FilterTableModel::checkedChanged, this, [=] {
                file->setColumnFilter(column, filterModel->checkedValues());
            });
            _filterDialog = new FilterDialog(filterModel, this);
            _filterDialog->setSizeGripEnabled(true);
            _filterDialog->exec();
        });

        connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, [=](int column) {
            file->requestFilter(column);
        });
    }

} // namespace gui
