#pragma once

#include "FilterDialog.h"
#include "LogFile.h"
#include "ThreadDispatcher.h"
#include "seer/LineParserRepository.h"
#include "seer/InstanceTracker.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QTableView>
#include <QStackedLayout>
#include <QLabel>
#include <memory>
#include <vector>

namespace gui {

struct OpenedLogFile {
    std::string path;
    std::unique_ptr<LogFile> file;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

    ThreadDispatcher _dispatcher;
    QTabWidget* _tabWidget;
    std::vector<OpenedLogFile> _logs;
    QStackedLayout* _centralLayout;
    QLabel* _dragAndDropTip;
    seer::LineParserRepository _repository;
    std::function<void()> _updateMenu;
    seer::InstanceTracker* _tracker;
    std::thread _trackerThread;

    void updateTabWidgetVisibility();
    void closeTab(int index);
    void interrupt(int index);
    void saveOpenedFilesToConfig();
    void addRecentFileToConfig(std::string path);
    void createMenu();
    QFont loadFont();
    void openFile();
    void closeCurrentTab();
    void clearFilters();
    void showAbout();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void openLog(std::string path, std::string parser = "");
    void setInstanceTracker(seer::InstanceTracker* tracker);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
};

} // namespace gui
