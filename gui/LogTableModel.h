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
        bool filterActive = false;
    };

    enum class HeaderDataRole {
        IsIndexed = Qt::UserRole,
        IsFilterActive,
        FirstLine,
    };

    class LogTableModel : public QAbstractTableModel {
        Q_OBJECT;

        seer::Index* _index = nullptr;
        seer::FileParser* _parser;
        std::vector<ColumnInfo> _columns;
        bool _showIndexedColumns = false;
        int _selectedFirstRow = -1;
        int _selectedLastRow = -1;

        void setSelection(int row, int last = -1);
        void extendSelection(int row);
        std::tuple<int, int> getSelection();

    public:
        LogTableModel(seer::FileParser* parser);
        void invalidate();
        void setFilterActive(int column, bool active);
        void setIndex(seer::Index* index);
        void showIndexedColumns();
        bool isSelected(int row);
        uint64_t lineOffset(uint64_t row);
        int findRow(uint64_t lineOffset);
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    signals:
        void selectionChanged();
    };

} // namespace gui
