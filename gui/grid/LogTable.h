#pragma once

#include "../LogTableModel.h"
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

    void setColumnWidth(int column, int width);

public:
    explicit LogTable(QFont font, QWidget* parent = nullptr);
    void setModel(LogTableModel* model);
    void setHist(const seer::Hist* hist);
    LogTableModel* model() const;
    FilterHeaderView* header() const;
    LogScrollArea* scrollArea() const;
    HistMap* histMap() const;
    void showHistMap();
    void setSearchHighlight(std::string text,
                            bool regex,
                            bool caseSensitive,
                            bool messageOnly);

signals:
    void requestFilter(int column);

public:
    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

} // namespace gui::grid
