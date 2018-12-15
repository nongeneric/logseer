#include "MainWindow.h"

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "SearchLine.h"
#include "grid/FilterHeaderView.h"
#include "grid/LogTable.h"
#include "seer/LineParserRepository.h"
#include "version.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStatusBar>
#include <fstream>
#include <boost/filesystem.hpp>

namespace gui {

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        _tabWidget = new QTabWidget(this);
        _tabWidget->setTabsClosable(true);

        connect(_tabWidget,
                &QTabWidget::tabCloseRequested,
                _tabWidget,
                &QTabWidget::removeTab);

        setCentralWidget(_tabWidget);
        setAcceptDrops(true);
        setWindowTitle(QString("logseer %0").arg(g_version));
        resize(800, 600);
    }

    void MainWindow::openLog(std::string path) {
        auto stream = std::make_unique<std::ifstream>(path);
        auto file = std::make_unique<LogFile>(
            std::move(stream), std::make_shared<seer::LineParserRepository>());

        auto table = new grid::LogTable();

        auto mainTableAndSearch = new QWidget();
        auto vbox = new QVBoxLayout();
        auto searchLine = new SearchLine();
        mainTableAndSearch->setLayout(vbox);

        connect(file.get(),
                &LogFile::parsingComplete,
                this,
                [=, file = file.get()] {
            searchLine->setStatus("Indexing...");
            table->setModel(file->logTableModel());
            file->index();
        }, Qt::QueuedConnection);

        connect(file.get(),
                &LogFile::parsingProgress,
                this,
                [=, file = file.get()] (auto progress) {
            searchLine->setProgress(progress);
        }, Qt::QueuedConnection);

        connect(file.get(),
                &LogFile::indexingComplete,
                this,
                [=, file = file.get()] {
            searchLine->setStatus("");
            searchLine->setProgress(-1);
        }, Qt::QueuedConnection);

        connect(file.get(),
                &LogFile::indexingProgress,
                this,
                [=, file = file.get()] (auto progress) {
            searchLine->setProgress(progress);
        }, Qt::QueuedConnection);

        searchLine->setStatus("Parsing...");
        file->parse();

        vbox->addWidget(table);
        vbox->addWidget(searchLine);
        vbox->setMargin(0);

        auto splitter = new QSplitter();
        splitter->setOrientation(Qt::Vertical);
        splitter->addWidget(mainTableAndSearch);

        auto searchTable = new grid::LogTable();
        splitter->addWidget(searchTable);

        connect(searchLine,
                &SearchLine::searchRequested,
                this,
                [=, file = file.get()] (std::string text, bool caseSensitive) {
            auto model = file->searchLogTableModel(text, caseSensitive);
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
