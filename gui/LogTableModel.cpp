#include "LogTableModel.h"

namespace gui {

    enum ColumnType {
        LineNumber = 0,
        Regular = 1
    };

    LogTableModel::LogTableModel(seer::FileParser* parser)
        : _parser(parser) {
        _columns.push_back({"#", false});
        for (auto& column : parser->lineParser()->getColumnFormats()) {
            _columns.push_back(
                {QString::fromStdString(column.header), column.indexed});
        }
    }

    void LogTableModel::invalidate() {
        endResetModel();
    }

    void LogTableModel::setFilterActive(int column, bool active) {
        _columns[column].filterActive = active;
    }

    void LogTableModel::setIndex(seer::Index* index) {
        _index = index;
        invalidate();
    }

    QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (section == -1 || orientation != Qt::Horizontal)
            return QVariant();
        if (role == (int)HeaderDataRole::IsIndexed) {
            return _columns[section].indexed;
        }
        if (role == (int)HeaderDataRole::IsFilterActive) {
            return _columns[section].filterActive;
        }
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            return _columns[section].name;
        }
        return QVariant();
    }

    int LogTableModel::rowCount([[maybe_unused]] const QModelIndex& parent) const {
        if (!_index)
            return _parser->lineCount();
        return _index->getLineCount();
    }

    int LogTableModel::columnCount([[maybe_unused]] const QModelIndex& parent) const {
        return _columns.size();
    }

    QVariant LogTableModel::data(const QModelIndex& index, int role) const {
        if (role != Qt::DisplayRole)
            return {};
        auto lineIndex = _index ? _index->mapIndex(index.row()) : index.row();
        if (index.column() == LineNumber)
            return QString("%0").arg(lineIndex);

        std::vector<std::string> line;
        _parser->readLine(lineIndex, line);
        size_t columnIndex = index.column() - Regular;
        if (columnIndex < line.size()) {
            return QString::fromStdString(line[columnIndex]);
        }
        return "###";
    }

} // namespace gui
