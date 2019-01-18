#pragma once

#include <QWidget>
namespace gui::grid {

    class LogTable;

    class LogTableView : public QWidget {
        Q_OBJECT

        LogTable* _table;
        int _rowHeight;

        void paintRow(QPainter* painter, int row, int y);
        int getRow(int y);

    public:
        explicit LogTableView(LogTable* parent);
        QSize sizeHint() const override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        int getRowY(int row);

    signals:

    public slots:

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
    };

} // namespace gui::grid
