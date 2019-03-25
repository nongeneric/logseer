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
        int _tabWidth = 4;

        void paintRow(QPainter* painter, int row, int y);
        int getRow(int y);
        void copyToClipboard();

    public:
        explicit LogTableView(QFont font, LogTable* parent);
        void setFirstRow(int row);
        int visibleRows();
        void setSearchHighlight(std::string text, bool regex, bool caseSensitive);

    signals:

    public slots:

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
    };

} // namespace gui::grid
