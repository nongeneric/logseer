#pragma once

#include "FilterDialog.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QTableView>

namespace gui {

    class MainWindow : public QMainWindow {
        Q_OBJECT

        QTabWidget* _tabWidget;
        FilterDialog* _filterDialog;

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        void openLog(std::string path);

    signals:

    public slots:
    };

} // namespace gui
