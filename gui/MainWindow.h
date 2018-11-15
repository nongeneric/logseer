#pragma once

#include "FilterDialog.h"
#include "LogFile.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QTableView>
#include <vector>
#include <memory>

namespace gui {

    class MainWindow : public QMainWindow {
        Q_OBJECT

        QTabWidget* _tabWidget;
        FilterDialog* _filterDialog;
        std::vector<std::unique_ptr<LogFile>> _logs;

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        void openLog(std::string path);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
    };

} // namespace gui
