#include "MainWindow.h"

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "grid/FilterHeaderView.h"
#include "seer/LineParserRepository.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <fstream>
#include <boost/filesystem.hpp>

namespace gui {

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        _tabWidget = new QTabWidget(this);
        setCentralWidget(_tabWidget);
        setAcceptDrops(true);
        resize(800, 600);
    }

    void MainWindow::openLog(std::string path) {
        auto stream = std::make_unique<std::ifstream>(path);
        auto file = std::make_unique<LogFile>(
            std::move(stream), std::make_shared<seer::LineParserRepository>());
        file->parse();

        auto table = new QTableView();
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

        connect(
            file.get(),
            &LogFile::filterRequested,
            this,
            [file = file.get(), this](auto filterModel, int column) {
                connect(filterModel, &FilterTableModel::checkedChanged, this, [=] {
                    file->setColumnFilter(column, filterModel->checkedValues());
                });
                _filterDialog = new FilterDialog(filterModel, this);
                _filterDialog->setSizeGripEnabled(true);
                _filterDialog->exec();
            });

        connect(table->horizontalHeader(),
                &QHeaderView::sectionClicked,
                this,
                [file = file.get()](int column) {
                    if (column != 0) {
                        file->requestFilter(column - 1);
                    }
                });

        auto fileName = boost::filesystem::path(path).stem().string();
        _tabWidget->addTab(table, QString::fromStdString(fileName));

        _logs.push_back(std::move(file));
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
        if (event->mimeData()->hasFormat("text/uri-list")) {
            event->acceptProposedAction();
        }
    }

    void MainWindow::dropEvent(QDropEvent* event) {
        if (event->mimeData()->hasUrls()) {
            auto uri = event->mimeData()->urls().first();
            openLog(uri.toLocalFile().toStdString());
        }
    }

} // namespace gui
