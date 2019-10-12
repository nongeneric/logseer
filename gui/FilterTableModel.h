#pragma once

#include "seer/Index.h"

#include <QAbstractTableModel>
#include <vector>
#include <string>
#include <set>

namespace gui {

    class FilterTableModel : public QAbstractTableModel {
        Q_OBJECT
        std::vector<seer::ColumnIndexInfo> _infos;
        std::vector<size_t> _visibleVec;
        std::set<size_t> _visibleSet;
        void updateVisibleVec();
        void emitAllChanged();

    public:
        FilterTableModel(std::vector<seer::ColumnIndexInfo> const& values);
        void selectAll();
        void selectNone();
        void selectFound();
        void search(std::string key);
        void invertSelection(std::vector<int> rows);
        int rowCount(const QModelIndex &parent) const override;
        int columnCount(const QModelIndex &parent) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        std::set<std::string> checkedValues() const;
        Qt::ItemFlags flags(const QModelIndex &index) const override;

    signals:
        void checkedChanged();
    };

} // namespace gui
