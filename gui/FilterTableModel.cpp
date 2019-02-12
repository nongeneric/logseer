#include "FilterTableModel.h"

#include <algorithm>

namespace gui {

    void FilterTableModel::updateVisibleVec() {
        _visibleVec.clear();
        for (auto& value : _visibleSet) {
            _visibleVec.push_back(value);
        }
    }

    void FilterTableModel::emitAllChanged() {
        emit dataChanged(index(0, 0), index(_infos.size(), 0), {Qt::CheckStateRole});
        emit checkedChanged();
    }

    FilterTableModel::FilterTableModel(std::vector<seer::ColumnIndexInfo> const& values) {
        _infos = values;
        for (auto i = 0u; i < _infos.size(); ++i) {
            _visibleSet.insert(i);
        }
        updateVisibleVec();
    }

    void FilterTableModel::selectAll() {
        for (auto& info : _infos) {
            info.checked = true;
        }
        emitAllChanged();
    }

    void FilterTableModel::selectNone() {
        for (auto& info : _infos) {
            info.checked = false;
        }
        emitAllChanged();
    }

    void FilterTableModel::selectFound() {
        for (auto i = 0u; i < _infos.size(); ++i) {
            _infos[i].checked = _visibleSet.find(i) != end(_visibleSet);
        }
        emitAllChanged();
    }

    void FilterTableModel::search(std::string key) {
        _visibleSet.clear();
        auto qKey = QString::fromStdString(key);
        for (auto i = 0u; i < _infos.size(); ++i) {
            auto qValue = QString::fromStdString(_infos[i].value);
            if (qValue.contains(qKey, Qt::CaseInsensitive)) {
                _visibleSet.insert(i);
            }
        }
        updateVisibleVec();
        endResetModel();
    }

    int FilterTableModel::rowCount(const QModelIndex&) const {
        return _visibleVec.size();
    }

    int FilterTableModel::columnCount(const QModelIndex&) const {
        return 3;
    }

    QVariant FilterTableModel::data(const QModelIndex& index, int role) const {
        auto& info = _infos[_visibleVec[index.row()]];
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            return info.checked ? Qt::Checked : Qt::Unchecked;
        } else if (index.column() == 1 && role == Qt::DisplayRole) {
            return QString::fromStdString(info.value);
        } else if (index.column() == 2 && role == Qt::DisplayRole) {
            return QString("%0").arg(info.count);
        }
        return {};
    }

    bool FilterTableModel::setData(const QModelIndex& index,
                                   const QVariant& value,
                                   int role) {
        if (role == Qt::CheckStateRole && index.column() == 0) {
            _infos[_visibleVec[index.row()]].checked = value.toBool();
            emit checkedChanged();
            return true;
        }
        return false;
    }

    QVariant FilterTableModel::headerData(int section,
                                          Qt::Orientation orientation,
                                          int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        switch (section) {
            case 0: return "";
            case 1: return "Value";
            case 2: return "Count";
        }
        assert(false);
        return "";
    }

    Qt::ItemFlags FilterTableModel::flags(const QModelIndex &index) const {
        auto baseFlags = QAbstractTableModel::flags(index);
        if (index.column() == 0) {
            baseFlags |= Qt::ItemIsUserCheckable;
        }
        return baseFlags;
    }

    std::set<std::string> FilterTableModel::checkedValues() const {
        std::set<std::string> values;
        for (auto i = 0u; i < _infos.size(); ++i) {
            if (_infos[i].checked) {
                values.insert(_infos[i].value);
            }
        }
        return values;
    }

} // namespace gui
