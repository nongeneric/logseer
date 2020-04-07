#pragma once

#include <QWidget>
#include <QRect>
#include "seer/Searcher.h"

namespace gui::grid {

class LogTable;

class LogTableView : public QWidget {
    Q_OBJECT

    LogTable* _table;
    int _rowHeight;
    int _firstRow = 0;
    std::unique_ptr<seer::ISearcher> _searcher;
    float _charWidth;
    bool _messageOnlyHighlight = false;
    bool _selectWords = false;

    void paintRow(QPainter* painter, int row, int y);
    int getRow(int y);
    std::tuple<int, int> getColumn(int x);
    void copyToClipboard(bool raw);

public:
    explicit LogTableView(QFont font, LogTable* parent);
    void setFirstRow(int row);
    int visibleRows();
    float charWidth() const;
    void setSearchHighlight(std::string text,
                            bool regex,
                            bool caseSensitive,
                            bool messageOnly);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

} // namespace gui::grid
