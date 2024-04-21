#pragma once

#include <QWidget>
#include <QRect>
#include <memory>
#include <seer/Searcher.h>
#include <gui/GraphemeMap.h>
#include <gui/grid/LruCache.h>
#include <gui/grid/RowColumn.h>
#include <gui/grid/CachedHighlightSearcher.h>

class QMenu;

namespace gui {

struct ColumnSelection;
struct IFontMetrics;

}

namespace gui::grid {

class LogTable;

struct LogicalPosition {
    int row = -1;
    int column = -1;
    int grapheme = -1;
};

class LogTableView : public QWidget {
    Q_OBJECT

    struct CacheEntry {
        std::shared_ptr<gui::GraphemeMap> gmap;
        QColor foreground;
    };

    LogTable* _table;
    int _rowHeight;
    int _firstRow = 0;
    std::optional<CachedHighlightSearcher> _cachedSearcher;
    bool _messageOnlyHighlight = false;
    bool _selectWords = false;
    std::unique_ptr<IFontMetrics> _gmapFontMetrics;
    LruCache<RowColumn, CacheEntry, RowColumnHash> _gmapCache;

    void paintRow(QPainter* painter, int row, int y);
    LogicalPosition getLogicalPosition(QPointF point);
    void copyToClipboard(bool raw);
    void addColumnExcludeActions(int column, int row, QMenu& menu);
    void addClearAllFiltersAction(QMenu& menu);
    std::shared_ptr<GraphemeMap> getGraphemeMap(int row, int column);

public:
    explicit LogTableView(QFont font, LogTable* parent);
    void setFirstRow(int row);
    int visibleRows();
    float textWidth(const QString& text) const;
    void setSearchHighlight(std::string text,
                            bool regex,
                            bool caseSensitive,
                            bool unicodeAware,
                            bool messageOnly);
    QString getSelectionText(const ColumnSelection& columnSelection);
    void invalidateCache();

signals:

public slots:

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

} // namespace gui::grid
