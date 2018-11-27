#pragma once

#include "seer/Index.h"
#include "seer/FileParser.h"
#include <QAbstractTableModel>
#include <vector>

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
        seer::Index* _index;
        seer::FileParser* _parser;
        std::vector<ColumnInfo> _columns;

    public:
        LogTableModel(seer::Index* index, seer::FileParser* parser);
        void invalidate();
        void setFilterActive(int column, bool active);
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    };

} // namespace gui
