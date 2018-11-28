#include "MainWindow.h"

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "SearchLine.h"
#include "grid/FilterHeaderView.h"
#include "grid/LogTable.h"
#include "seer/LineParserRepository.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QVBoxLayout>
#include <QSplitter>
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

        auto table = new grid::LogTable();
        table->setModel(file->logTableModel());

        auto mainTableAndSearch = new QWidget();
        auto vbox = new QVBoxLayout();
        auto searchLine = new SearchLine();
        mainTableAndSearch->setLayout(vbox);

        vbox->addWidget(table);
        vbox->addWidget(searchLine);

        auto splitter = new QSplitter();
        splitter->setOrientation(Qt::Vertical);
        splitter->addWidget(mainTableAndSearch);

        auto searchTable = new grid::LogTable();
        splitter->addWidget(searchTable);

        connect(searchLine,
                &SearchLine::requestSearch,
                this,
                [=, file = file.get()] (std::string text) {
            auto model = file->searchLogTableModel(text);
            searchTable->setModel(model);
        });

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

        connect(table,
                &grid::LogTable::requestFilter,
                this,
                [file = file.get()](int column) { file->requestFilter(column); });

        auto fileName = boost::filesystem::path(path).stem().string();
        _tabWidget->addTab(splitter, QString::fromStdString(fileName));

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
