#include "MainWindow.h"

#include "FilterTableModel.h"
#include "LogTableModel.h"
#include "SearchLine.h"
#include "Config.h"
#include "grid/LogTable.h"
#include "grid/FilterHeaderView.h"
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
#include "seer/bformat.h"

namespace gui {

    void copySectionSizes(grid::FilterHeaderView* from, grid::FilterHeaderView* to) {
        assert(from->size() == to->size());
        for (auto i = 0; i < from->count(); ++i) {
            to->resizeSection(i, from->sectionSize(i));
        }
    }

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
            copySectionSizes(table->header(), searchTable->header());
        }

        if (file->isState(sm::ParsingState)) {
            searchLine->setStatus("Parsing...");
        } else if (file->isState(sm::IndexingState)) {
            searchLine->setStatus("Indexing...");
            table->setModel(file->logTableModel());
            searchLine->setSearchEnabled(true);
        } else if (file->isState(sm::SearchingState)) {
            searchLine->setStatus("Searching...");
        } else if (file->isState(sm::CompleteState)) {
            auto status = searchModel ? bformat("%d matches found",
                                                searchModel->rowCount({}))
                                      : "";
            searchLine->setStatus(status);
            searchLine->setProgress(-1);
            searchLine->setSearchEnabled(true);
        } else if (file->isState(sm::InterruptedState)) {
            close();
        }
    }

    void MainWindow::updateTabWidgetVisibility() {
        _centralLayout->setCurrentWidget(_tabWidget->count() == 0
                                             ? _dragAndDropTip
                                             : static_cast<QWidget*>(_tabWidget));
    }

    void MainWindow::closeTab(int index) {
        _tabWidget->removeTab(index);
        _logs.erase(begin(_logs) + index);
        updateTabWidgetVisibility();
    }

    void MainWindow::interrupt(int index) {
        _logs[index].file->interrupt();
    }

    void MainWindow::saveOpenedFilesToConfig() {
        auto config = g_Config.sessionConfig();
        config.openedFiles.clear();
        for (auto& logFile : _logs) {
            config.openedFiles.push_back(logFile.path);
        }
        g_Config.save(config);
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

        _dragAndDropTip = new QLabel("Drag & Drop Files Here to Open");
        _dragAndDropTip->setAlignment(Qt::AlignCenter);
        _centralLayout = new QStackedLayout();
        _centralLayout->addWidget(_dragAndDropTip);
        _centralLayout->addWidget(_tabWidget);

        auto centralWidget = new QWidget(this);
        centralWidget->setLayout(_centralLayout);

        setCentralWidget(centralWidget);
        setAcceptDrops(true);
        setWindowTitle(QString("logseer %0%1").arg(g_version).arg(g_debug ? " [DEBUG]" : ""));
        resize(800, 600);
        updateTabWidgetVisibility();
    }

    void MainWindow::openLog(std::string path) {
        seer::log_infof("opening [%s]", path);

        if (boost::filesystem::is_directory(path)) {
            seer::log_info("attempted to open a directory, ignoring");
            return;
        }

        if (!boost::filesystem::exists(path)) {
            seer::log_info("attempted to open a nonexisting file, ignoring");
            return;
        }

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
        const auto& searchConfig = g_Config.searchConfig();
        auto searchLine = new SearchLine(searchConfig.regex,
                                         searchConfig.caseSensitive,
                                         searchConfig.messageOnly);
        mainTableAndSearch->setLayout(vbox);

        auto searchTable = new grid::LogTable(font);

        connect(file.get(), &LogFile::stateChanged, this, [=, file = file.get()] {
            handleStateChanged(file, table, searchTable, searchLine, [=] {
                auto it = std::find_if(begin(_logs), end(_logs), [=] (auto& x) {
                    return x.file.get() == file;
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
                &SearchLine::regexChanged,
                this,
                [&] (bool regex) {
            auto config = g_Config.searchConfig();
            config.regex = regex;
            g_Config.save(config);
        });

        connect(searchLine,
                &SearchLine::messageOnlyChanged,
                this,
                [&] (bool messageOnly) {
            auto config = g_Config.searchConfig();
            config.messageOnly = messageOnly;
            g_Config.save(config);
        });

        connect(searchLine,
                &SearchLine::searchRequested,
                this,
                [=, file = file.get()] (std::string text, bool regex, bool caseSensitive, bool messageOnly) {
            if (text.empty())
                return;
            file->search(text, regex, caseSensitive, messageOnly);
            searchTable->setSearchHighlight(text, regex, caseSensitive, messageOnly);
            table->setSearchHighlight(text, regex, caseSensitive, messageOnly);
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
        auto toolTip = bformat("%s\nparser type: %s", path.c_str(), lineParser->name());
        _tabWidget->setTabToolTip(index, QString::fromStdString(toolTip));

        _logs.push_back({path, std::move(file)});
        updateTabWidgetVisibility();
        saveOpenedFilesToConfig();
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
        saveOpenedFilesToConfig();
        std::vector<LogFile*> logs;
        for (auto& logFile : _logs) {
            logs.push_back(logFile.file.get());
        }
        for (auto& logFile : logs) {
            logFile->interrupt();
        }
    }

} // namespace gui
