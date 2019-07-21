#pragma once

#include "FilterDialog.h"
#include "LogFile.h"
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

        QTabWidget* _tabWidget;
        FilterDialog* _filterDialog;
        std::vector<OpenedLogFile> _logs;
        QStackedLayout* _centralLayout;
        QLabel* _dragAndDropTip;

        void updateTabWidgetVisibility();
        void closeTab(int index);
        void interrupt(int index);
        void saveOpenedFilesToConfig();
        QFont loadFont();

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        void openLog(std::string path);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
    };

} // namespace gui
