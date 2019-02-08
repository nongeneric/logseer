#pragma once

#include <QWidget>
#include <QRect>

namespace gui::grid {

    class LogTable;

    class LogTableView : public QWidget {
        Q_OBJECT

        LogTable* _table;
        int _rowHeight;
        int _firstRow = 0;
        std::string _searchText;
        bool _searchCaseSensitive;

        void paintRow(QPainter* painter, int row, int y);
        int getRow(int y);

    public:
        explicit LogTableView(LogTable* parent);
        void setFirstRow(int row);
        int visibleRows();
        void setSearchHighlight(std::string text, bool caseSensitive);

    signals:

    public slots:

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
    };

} // namespace gui::grid
