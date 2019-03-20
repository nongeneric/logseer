#include "MainWindow.h"

#include "FilterTableModel.h"
#include "LogTableModel.h"
#include "SearchLine.h"
#include "Config.h"
#include "grid/FilterHeaderView.h"
#include "grid/LogTable.h"
#include "seer/LineParserRepository.h"
#include "seer/Log.h"
#include "version.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <boost/filesystem.hpp>
#include <fstream>

namespace gui {

    void handleStateChanged(LogFile* file,
                            grid::LogTable* table,
                            grid::LogTable* searchTable,
                            SearchLine* searchLine,
                            std::function<void()> close) {
        searchLine->setSearchEnabled(false);
        auto searchModel = file->searchLogTableModel();
        if (searchModel) {
            searchTable->setModel(searchModel);
            table->setHist(file->searchHist());
        }

        if (file->isState(sm::ParsingState)) {
            searchLine->setStatus("Parsing...");
        } else if (file->isState(sm::IndexingState)) {
            searchLine->setStatus("Indexing...");
            table->setModel(file->logTableModel());
            searchLine->setSearchEnabled(true);
        } else if (file->isState(sm::SearchingState)) {
            searchLine->setStatus("Searching...");
            searchLine->setProgress(-1);
        } else if (file->isState(sm::CompleteState)) {
            auto status = searchModel ? seer::ssnprintf("%d matches found",
                                                        searchModel->rowCount({}))
                                      : "";
            searchLine->setStatus(status);
            searchLine->setProgress(-1);
            searchLine->setSearchEnabled(true);
        } else if (file->isState(sm::InterruptedState)) {
            close();
        }
    }

    void MainWindow::closeTab(int index) {
        _tabWidget->removeTab(index);
        _logs.erase(begin(_logs) + index);
    }

    void MainWindow::interrupt(int index) {
        _logs[index]->interrupt();
    }

    QFont MainWindow::loadFont() {
        QFont font;
        auto const& config = g_Config.fontConfig();
        font.setFamily(QString::fromStdString(config.name));
        font.setPointSize(config.size);
        return font;
    }

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        _tabWidget = new QTabWidget(this);
        _tabWidget->setTabsClosable(true);

        connect(_tabWidget,
                &QTabWidget::tabCloseRequested,
                this,
                &MainWindow::interrupt);

        setCentralWidget(_tabWidget);
        setAcceptDrops(true);
        setWindowTitle(QString("logseer %0").arg(g_version));
        resize(800, 600);
    }

    void MainWindow::openLog(std::string path) {
        seer::log_infof("opening [%s]", path);
        auto stream = std::make_unique<std::ifstream>(path, std::ios_base::binary);
        assert(stream->is_open());

        seer::LineParserRepository repository;
        for (auto& config : g_Config.regexConfigs()) {
            repository.addRegexParser(config.name, config.priority, config.json);
        }

        auto lineParser = repository.resolve(*stream);
        auto file = std::make_unique<LogFile>(std::move(stream), lineParser);

        auto font = loadFont();
        auto table = new grid::LogTable(font);
        table->showHistMap();

        auto mainTableAndSearch = new QWidget();
        auto vbox = new QVBoxLayout();
        auto searchLine = new SearchLine(g_Config.searchConfig().caseSensitive);
        mainTableAndSearch->setLayout(vbox);

        auto searchTable = new grid::LogTable(font);

        connect(file.get(), &LogFile::stateChanged, this, [=, file = file.get()] {
            handleStateChanged(file, table, searchTable, searchLine, [=] {
                auto it = std::find_if(begin(_logs), end(_logs), [=] (auto& x) {
                    return x.get() == file;
                });
                assert(it != end(_logs));
                this->closeTab(std::distance(begin(_logs), it));
            });
        });

        connect(file.get(),
                &LogFile::progressChanged,
                this,
                [=, file = file.get()] (auto progress) {
            searchLine->setProgress(progress);
        });

        file->parse();

        vbox->addWidget(table);
        vbox->addWidget(searchLine);
        vbox->setMargin(0);

        auto splitter = new QSplitter();
        splitter->setOrientation(Qt::Vertical);
        splitter->addWidget(mainTableAndSearch);
        splitter->addWidget(searchTable);
        splitter->setSizes({height() * 2/3, height() * 1/3});

        connect(searchLine,
                &SearchLine::caseSensitiveChanged,
                this,
                [&] (bool caseSensitive) {
            auto config = g_Config.searchConfig();
            config.caseSensitive = caseSensitive;
            g_Config.save(config);
        });

        connect(searchLine,
                &SearchLine::searchRequested,
                this,
                [=, file = file.get()] (std::string text, bool caseSensitive) {
            file->search(text, caseSensitive);
            searchTable->setSearchHighlight(text, caseSensitive);
            table->setSearchHighlight(text, caseSensitive);
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
        auto index = _tabWidget->addTab(splitter, QString::fromStdString(fileName));
        auto toolTip = seer::ssnprintf("%s\nparser type: %s", path.c_str(), lineParser->name());
        _tabWidget->setTabToolTip(index, QString::fromStdString(toolTip));

        _logs.push_back(std::move(file));
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
        if (event->mimeData()->hasFormat("text/uri-list")) {
            event->acceptProposedAction();
        }
    }

    void MainWindow::dropEvent(QDropEvent* event) {
        if (event->mimeData()->hasUrls()) {
            for (auto uri : event->mimeData()->urls()) {
                openLog(uri.toLocalFile().toStdString());
            }
        }
    }

    void MainWindow::closeEvent(QCloseEvent*) {
        std::vector<LogFile*> logs;
        for (auto& logFile : _logs) {
            logs.push_back(logFile.get());
        }
        for (auto& logFile : logs) {
            logFile->interrupt();
        }
    }

} // namespace gui
