#pragma once

#include "FilterDialog.h"
#include "LogFile.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QTableView>
#include <memory>
#include <vector>

namespace gui {

    class MainWindow : public QMainWindow {
        Q_OBJECT

        QTabWidget* _tabWidget;
        FilterDialog* _filterDialog;
        std::vector<std::unique_ptr<LogFile>> _logs;

        void closeTab(int index);
        void interrupt(int index);

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        void openLog(std::string path);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
    };

} // namespace gui
