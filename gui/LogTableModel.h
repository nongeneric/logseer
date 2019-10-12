#pragma once

#include "seer/Index.h"
#include "seer/FileParser.h"
#include <QAbstractTableModel>
#include <vector>
#include <tuple>

namespace gui {

    struct ColumnInfo {
        QString name;
        bool indexed = false;
        bool autosize = false;
        bool filterActive = false;
        int maxWidth = -1;
    };

    enum class HeaderDataRole {
        IsIndexed = Qt::UserRole,
        IsFilterActive,
        FirstLine,
        Autosize
    };

    class LogTableModel : public QAbstractTableModel {
        Q_OBJECT;

        seer::Index* _index = nullptr;
        seer::FileParser* _parser;
        std::vector<ColumnInfo> _columns;
        bool _showIndexedColumns = false;
        int _selectedFirstRow = -1;
        int _selectedLastRow = -1;

    public:
        using LineHandler = std::function<void(const std::string&)>;

        LogTableModel(seer::FileParser* parser);
        void invalidate();
        void setFilterActive(int column, bool active);
        void setIndex(seer::Index* index);
        void showIndexedColumns();
        void copyRawLines(uint64_t begin, uint64_t end, LineHandler accept);
        void copyLines(uint64_t begin, uint64_t end, LineHandler accept);
        bool isSelected(int row);
        uint64_t lineOffset(uint64_t row) const;
        int findRow(uint64_t lineOffset);
        int maxColumnWidth(int column);
        void setColumnWidths(std::vector<int> widths);
        std::tuple<int, int> getSelection();
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        void setSelection(int row, int last = -1);
        void extendSelection(int row);

    signals:
        void selectionChanged();
        void columnWidthsChanged();
    };

} // namespace gui
