#include "MainWindow.h"

#include "FilterTableModel.h"
#include "LogTableModel.h"
#include "SearchLine.h"
#include "Config.h"
#include "grid/LogTable.h"
#include "grid/FilterHeaderView.h"
#include "seer/Log.h"
#include "version.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QActionGroup>
#include <QMenuBar>
#include <QMenu>
#include <QActionGroup>
#include <QFileDialog>
#include <QShortcut>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>

namespace gui {

void copySectionSizes(grid::FilterHeaderView* from, grid::FilterHeaderView* to) {
    assert(from->count() == to->count());
    for (auto i = 0; i < from->count(); ++i) {
        to->resizeSection(i, from->sectionSize(i));
    }
}

void handleStateChanged(LogFile* file,
                        grid::LogTable* table,
                        grid::LogTable* searchTable,
                        SearchLine* searchLine,
                        std::function<void()> updateMenu,
                        std::function<void()> close) {
    searchLine->setSearchEnabled(false);
    auto searchModel = file->searchLogTableModel();
    searchTable->setModel(searchModel);
    if (searchModel) {
        table->setHist(file->searchHist());
        copySectionSizes(table->header(), searchTable->header());
    } else {
        table->setHist(nullptr);
        table->setSearchHighlight("", false, false, false, false);
    }

    if (file->isState(sm::IndexingState)) {
        searchLine->setStatus("Indexing...");
        table->setModel(nullptr);
        searchLine->setSearchEnabled(false);
        searchLine->setSearchButtonTitle(SearchButtonTitle::Search);
    } else if (file->isState(sm::SearchingState)) {
        searchLine->setStatus("Searching...");
        searchLine->setSearchEnabled(true);
        searchLine->setSearchButtonTitle(SearchButtonTitle::Abort);
    } else if (file->isState(sm::CompleteState)) {
        table->setModel(file->logTableModel());
        auto status = searchModel ? fmt::format("{} matches found", searchModel->rowCount({})) : "";
        searchLine->setStatus(status);
        searchLine->setProgress(-1);
        searchLine->setSearchEnabled(true);
        searchLine->setSearchButtonTitle(SearchButtonTitle::Search);
        updateMenu();
    } else if (file->isState(sm::InterruptedState)) {
        close();
    }
}

void MainWindow::updateTabWidgetVisibility() {
    _centralLayout->setCurrentWidget(_tabWidget->count() == 0
                                         ? _dragAndDropTip
                                         : static_cast<QWidget*>(_tabWidget));
    _updateMenu();
}

void MainWindow::closeTab(int index) {
    _filterDialog.close();
    addRecentFileToConfig(_logs[index].path);
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
        config.openedFiles.push_back({logFile.path, logFile.file->lineParser()->name()});
    }
    g_Config.save(config);
}

void MainWindow::addRecentFileToConfig(std::string path) {
    auto config = g_Config.sessionConfig();
    config.recentFiles.erase(
        std::remove_if(begin(config.recentFiles),
                       end(config.recentFiles),
                       [&](auto& p) { return p == path || !std::filesystem::exists(p); }),
        end(config.recentFiles));
    config.recentFiles.insert(begin(config.recentFiles), path);
    g_Config.save(config);
}

void MainWindow::createMenu() {
    auto fileMenu = menuBar()->addMenu("&File");
    auto editMenu = menuBar()->addMenu("&Edit");
    auto reloadAction = new QAction("&Reload", this);
    reloadAction->setShortcut(QKeySequence::Refresh);
    connect(reloadAction, &QAction::triggered, this, [this] {
        auto index = _tabWidget->currentIndex();
        auto& log = _logs[index];
        auto stream = std::make_shared<std::ifstream>(log.path, std::ios_base::binary);
        _filterDialog.close();
        log.file->reload(stream);
    });
    editMenu->addAction(reloadAction);

    auto clearFiltersAction = new QAction("&Clear filters", this);
    clearFiltersAction->setShortcut(QKeySequence::Replace);
    connect(clearFiltersAction, &QAction::triggered, this, &MainWindow::clearFilters);
    editMenu->addAction(clearFiltersAction);

    auto filtersMenu = editMenu->addMenu("&Filters");

    auto parsersMenu = editMenu->addMenu("&Parser");
    auto parserGroup = new QActionGroup(this);
    parserGroup->setExclusive(true);
    std::map<std::string, QAction*> parserActions;
    for (const auto& [priority, parser] : _repository.parsers()) {
        auto action = new QAction(QString::fromStdString(parser->name()), this);
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, [this, parser = parser] {
            auto index = _tabWidget->currentIndex();
            auto& log = _logs[index];
            auto stream = std::make_shared<std::ifstream>(log.path, std::ios_base::binary);
            auto lineParser = resolveByName(&_repository, parser->name());
            _filterDialog.close();
            log.file->reload(stream, lineParser);
        });
        parsersMenu->addAction(action);
        parserGroup->addAction(action);
        parserActions[parser->name()] = action;
    }

    auto helpMenu = menuBar()->addMenu("&Help");
    auto aboutAction = new QAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);

    _updateMenu = [=, this] {
        fileMenu->clear();
        auto openAction = new QAction("&Open", this);
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
        fileMenu->addAction(openAction);
        auto closeAction = new QAction("&Close Tab", this);
        closeAction->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | Qt::Key_W));
        connect(closeAction, &QAction::triggered, this, &MainWindow::closeCurrentTab);
        fileMenu->addAction(closeAction);

        fileMenu->addSeparator();
        for (auto recentFile : g_Config.sessionConfig().recentFiles) {
            auto action = new QAction(QString::fromStdString(recentFile), this);
            connect(action, &QAction::triggered, this, [=, this] {
                openLog(recentFile);
            });
            fileMenu->addAction(action);
        }

        fileMenu->addSeparator();
        auto exitAction = new QAction("&Exit", this);
        connect(exitAction, &QAction::triggered, this, &MainWindow::close);
        fileMenu->addAction(exitAction);

        auto index = _tabWidget->currentIndex();
        auto noTabsOpened = index == -1;
        reloadAction->setEnabled(!noTabsOpened);
        clearFiltersAction->setEnabled(!noTabsOpened);
        parsersMenu->setEnabled(!noTabsOpened);
        filtersMenu->setEnabled(!noTabsOpened);
        closeAction->setEnabled(!noTabsOpened);

        if (noTabsOpened)
            return;

        auto& log = _logs[index];

        filtersMenu->clear();
        auto indexColumn = 0;
        auto model = log.file->logTableModel();
        for (auto i = 0; i < model->columnCount({}); ++i) {
            if (!model->headerData(i, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool())
                continue;
            auto columnName = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            auto action = new QAction(columnName, this);
            action->setShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + Qt::Key(indexColumn))));
            connect(action, &QAction::triggered, this, [=, file=log.file.get()] {
                file->requestFilter(i);
            });
            filtersMenu->addAction(action);
            indexColumn++;
        }

        parserActions.at(log.file->lineParser()->name())->setChecked(true);
    };

    _updateMenu();
}

QFont MainWindow::loadFont() {
    QFont font;
    auto const& config = g_Config.fontConfig();
    font.setFamily(QString::fromStdString(config.name));
    font.setPointSize(config.size);
    return font;
}

void MainWindow::openFile() {
    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Log");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    auto index = _tabWidget->currentIndex();
    if (index != -1) {
        auto directory = std::filesystem::path(_logs[index].path).parent_path().string();
        dialog.setDirectory(QString::fromStdString(directory));
    }
    if (dialog.exec()) {
        for (auto path : dialog.selectedFiles()) {
            openLog(path.toStdString());
        }
    }
}

void MainWindow::closeCurrentTab() {
    auto index = _tabWidget->currentIndex();
    assert(index != -1);
    _logs[index].file->interrupt();
}

void MainWindow::clearFilters() {
    auto index = _tabWidget->currentIndex();
    assert(index != -1);
    _logs[index].file->clearFilters();
}

void MainWindow::showAbout() {
    auto debugBuild = g_debug ? " [DEBUG]" : "";
    auto title = fmt::format("About {}", g_name);
    auto text = fmt::format("<b>{0} {1}{2}</b><p>"
                            "Config: <a href='file://{3}'>{3}</a><p>"
                            "GitHub: <a href='{4}'>{4}</a>",
                            g_name,
                            g_version,
                            debugBuild,
                            g_Config.getConfigDirectory().string(),
                            "https://github.com/nongeneric/logseer");
    QMessageBox::about(this, QString::fromStdString(title), QString::fromStdString(text));
}

int MainWindow::findTab(const LogFile* file) {
    auto it = std::find_if(begin(_logs), end(_logs), [=](auto& x) { return x.file.get() == file; });
    return it == end(_logs) ? -1 : std::distance(begin(_logs), it);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), _dispatcher(this) {
    _tabWidget = new QTabWidget(this);
    _tabWidget->setTabsClosable(g_Config.generalConfig().showCloseTabButton);
    _tabWidget->setMovable(true);
    _updateMenu = []{};

    connect(_tabWidget,
            &QTabWidget::tabCloseRequested,
            this,
            &MainWindow::interrupt);

    connect(_tabWidget, &QTabWidget::currentChanged, this, [this] {
        _filterDialog.close();
        _updateMenu();
    });

    connect(_tabWidget->tabBar(), &QTabBar::tabMoved, this, [this](int from, int to) {
        std::swap(_logs[from], _logs[to]);
    });

    _dragAndDropTip = new QLabel("Drag & Drop Files Here to Open", this);
    _dragAndDropTip->setAlignment(Qt::AlignCenter);
    _centralLayout = new QStackedLayout(this);
    _centralLayout->addWidget(_dragAndDropTip);
    _centralLayout->addWidget(_tabWidget);

    auto centralWidget = new QWidget(this);
    centralWidget->setLayout(_centralLayout);

    setCentralWidget(centralWidget);
    setAcceptDrops(true);
    setWindowTitle(QString("logseer%0").arg(g_debug ? " [DEBUG]" : ""));
    setWindowIcon(QIcon(":/logseer.svg"));
    resize(800, 600);
    updateTabWidgetVisibility();

    for (auto& config : g_Config.regexConfigs()) {
        auto error = _repository.addRegexParser(config.name, config.priority, config.json);
        if (error) {
            auto title = fmt::format("Error loading regex config: {}", config.name);
            QMessageBox::warning(
                this, QString::fromStdString(title), QString::fromStdString(*error));
        }
    }

    createMenu();
}

MainWindow::~MainWindow() {
    _tracker->stop();
    if (_trackerThread.joinable())
        _trackerThread.join();
}

void MainWindow::openLog(std::string path, std::string parser) {
    seer::log_infof("opening [{}]", path);

    if (std::filesystem::is_directory(path)) {
        seer::log_info("attempted to open a directory, ignoring");
        return;
    }

    if (!std::filesystem::exists(path)) {
        seer::log_info("attempted to open a nonexisting file, ignoring");
        return;
    }

    auto stream = std::make_unique<std::ifstream>(path, std::ios_base::binary);
    assert(stream->is_open());

    auto lineParser = parser.empty() ? _repository.resolve(*stream) : resolveByName(&_repository, parser);
    auto file = std::make_unique<LogFile>(std::move(stream), lineParser);

    auto font = loadFont();
    auto table = new grid::LogTable(file.get(), font);
    table->showHistMap();

    auto mainTableAndSearch = new QWidget();
    auto vbox = new QVBoxLayout();
    const auto& searchConfig = g_Config.searchConfig();
    auto searchLine = new SearchLine(searchConfig.regex,
                                     searchConfig.caseSensitive,
                                     searchConfig.unicodeAware,
                                     searchConfig.messageOnly);
    mainTableAndSearch->setLayout(vbox);

    auto searchTable = new grid::LogTable(nullptr, font);

    connect(file.get(), &LogFile::stateChanged, this, [=, this, file = file.get()] {
        auto index = findTab(file);
        if (index != -1) {
            auto toolTip = fmt::format("{}\nparser type: {}", path.c_str(), file->lineParser()->name());
            _tabWidget->setTabToolTip(index, QString::fromStdString(toolTip));
        }
        handleStateChanged(file, table, searchTable, searchLine, _updateMenu, [=, this] {
            assert(index != -1);
            closeTab(index);
        });
    });

    connect(file.get(),
            &LogFile::progressChanged,
            this,
            [=, file = file.get()] (auto progress) {
        searchLine->setProgress(progress);
    });

    file->index();

    vbox->addWidget(table);
    vbox->addWidget(searchLine);
    vbox->setContentsMargins(0, 0, 0, 0);

    auto splitter = new QSplitter();
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(mainTableAndSearch);
    splitter->addWidget(searchTable);
    splitter->setSizes({height() * 2/3, height() * 1/3});

    _filterDialog.setSizeGripEnabled(true);
    _filterDialog.resize(450, 450);

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

    connect(
        searchLine,
        &SearchLine::searchRequested,
        this,
        [=, file = file.get()](
            std::string text, bool regex, bool caseSensitive, bool unicodeAware, bool messageOnly) {
            if (text.empty()) {
                table->setHist(nullptr);
                searchTable->setModel(nullptr);
                searchLine->setStatus("");
            } else {
                file->search(text, regex, caseSensitive, unicodeAware, messageOnly);
            }
            searchTable->setSearchHighlight(text, regex, caseSensitive, unicodeAware, messageOnly);
            table->setSearchHighlight(text, regex, caseSensitive, unicodeAware, messageOnly);
        });

    connect(
        file.get(),
        &LogFile::filterRequested,
        this,
        [file = file.get(), this](auto filterModel, int column, std::string header) {
            connect(filterModel.get(), &FilterTableModel::checkedChanged, this, [=] {
                file->setColumnFilter(column, filterModel->checkedValues());
            });
            _filterDialog.setModel(filterModel);
            _filterDialog.setWindowTitle(QString::fromStdString(fmt::format("[{}] filter", header)));
            _filterDialog.show();
        });

    connect(table,
            &grid::LogTable::requestFilter,
            this,
            [file = file.get()](int column) { file->requestFilter(column); });

    _logs.push_back({path, std::move(file)});

    auto fileName = std::filesystem::path(path).stem().string();
    if (fileName.empty()) {
        fileName = std::filesystem::path(path).filename().string();
    }
    auto index = _tabWidget->addTab(splitter, QString::fromStdString(fileName));
    _tabWidget->setCurrentIndex(index);

    updateTabWidgetVisibility();
    saveOpenedFilesToConfig();
}

void MainWindow::setInstanceTracker(seer::InstanceTracker* tracker) {
    seer::log_infof("{} called", __func__);
    assert(tracker);
    _tracker = tracker;
    _trackerThread = std::thread([this] {
        while (auto message = _tracker->waitMessage()) {
            seer::log_infof("posting file path to UI thread");
            _dispatcher.postToUIThread([=, this] {
                seer::log_infof("received file path through InstanceTracker: {}", *message);
                openLog(*message);
                activateWindow();
            });
        }
    });
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
