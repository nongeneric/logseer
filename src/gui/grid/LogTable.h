#pragma once

#include "gui/LogTableModel.h"
#include "gui/LogFile.h"
#include "FilterHeaderView.h"
#include "LogScrollArea.h"
#include "HistMap.h"
#include <QWidget>

namespace gui::grid {

class LogTableView;

class LogTable : public QWidget {
    Q_OBJECT

    LogTableModel* _model = nullptr;
    FilterHeaderView* _header = nullptr;
    LogTableView* _view = nullptr;
    LogScrollArea* _scrollArea = nullptr;
    HistMap* _histMap = nullptr;
    bool _showHistMap = false;
    gui::LogFile* _logFile;

    void setColumnWidth(int column, int longestColumnIndex);

public:
    LogTable(gui::LogFile* logFile, QFont font, QWidget* parent = nullptr);
    void setModel(LogTableModel* model);
    void setHist(const seer::Hist* hist);
    LogTableModel* model() const;
    FilterHeaderView* header() const;
    LogScrollArea* scrollArea() const;
    HistMap* histMap() const;
    LogFile* logFile() const;
    void showHistMap();
    void setSearchHighlight(std::string text,
                            bool regex,
                            bool caseSensitive,
                            bool unicodeAware,
                            bool messageOnly);
    void updateMessageWidth(int width);

signals:
    void requestFilter(int column);

public:
    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};

} // namespace gui::grid
