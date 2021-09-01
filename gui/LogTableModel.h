#pragma once

#include "seer/Index.h"
#include "seer/FileParser.h"
#include "GraphemeMap.h"
#include <QAbstractTableModel>
#include <vector>
#include <tuple>
#include <variant>
#include <optional>

namespace gui {

struct ColumnInfo {
    ColumnInfo(QString name, bool indexed, bool autosize)
        : name(name), indexed(indexed), autosize(autosize) {}

    QString name;
    bool indexed = false;
    bool autosize = false;
    bool filterActive = false;
    seer::ColumnWidth maxWidth;
};

enum class HeaderDataRole {
    IsIndexed = Qt::UserRole,
    IsFilterActive,
    FirstLine,
    IsAutosize,
    LongestColumnIndex
};

struct RowSelection {
    int first = -1;
    int last = -1;

    bool isSelected(int row) const {
        return first <= row && row <= last;
    }

    int size() const {
        return last - first + 1;
    }
};

struct ColumnSelection {
    int row = -1;
    int column = -1;
    int first = -1;
    int last = -1;

    int size() const {
        return last - first + 1;
    }
};

using LogTableSelection = std::variant<std::monostate, RowSelection, ColumnSelection>;

class SelectionTracker {
    int _first = -1;
    int _last = -1;

public:
    void set(int index) {
        _first = index;
        _last = index;
    }

    bool extend(int index) {
        if (_last == index)
            return false;
        _last = index;
        return true;
    }

    int left() const {
        return std::min(_first, _last);
    }

    int right() const {
        return std::max(_first, _last);
    }

    int size() const {
        return right() - left() + 1;
    }

    int original() const {
        return _first;
    }
};

class LogTableModel : public QAbstractTableModel {
    Q_OBJECT;

    seer::Index* _index = nullptr;
    seer::FileParser* _parser;
    std::vector<ColumnInfo> _columns;
    bool _showIndexedColumns = false;

    int _selectedColumn = -1;
    SelectionTracker _selectedRow;
    SelectionTracker _selectedChar;
    bool _selectionExtended = true;

public:
    using LineHandler = std::function<void(const std::string&)>;

    LogTableModel(seer::FileParser* parser);
    void invalidate();
    void setFilterActive(int column, bool active);
    void setIndex(seer::Index* index);
    void showIndexedColumns();
    void copyRawLines(uint64_t begin, uint64_t end, LineHandler accept);
    void copyLines(uint64_t begin, uint64_t end, LineHandler accept);
    uint64_t lineOffset(uint64_t row) const;
    int findRow(uint64_t lineOffset);
    void setColumnWidths(std::vector<seer::ColumnWidth> widths);
    virtual int rowCount(const QModelIndex& parent) const override;
    int unfilteredRowCount() const;
    virtual int columnCount(const QModelIndex& parent) const override;
    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void setSelection(int row, int column, int index);
    void forceColumnSelection();
    bool extendSelection(int row, int column, int index);
    std::optional<RowSelection> getRowSelection() const;
    LogTableSelection getSelection() const;
    std::vector<std::string> values(int column) const;

signals:
    void selectionChanged();
};

} // namespace gui
