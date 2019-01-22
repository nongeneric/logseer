#include "LogTableModel.h"

#include <boost/range/irange.hpp>

namespace gui {

    enum ColumnType {
        LineNumber = 0,
        Regular = 1
    };

    void LogTableModel::setSelection(int row, int last) {
        if (row == -1) {
            _selectedFirstRow = -1;
            _selectedLastRow = -1;
        } else {
            _selectedFirstRow = row;
            _selectedLastRow = last == -1 ? row + 1 : last;
        }
        emit selectionChanged();
    }

    void LogTableModel::extendSelection(int row) {
        _selectedLastRow = row + 1;
        if (_selectedFirstRow < _selectedLastRow) {
            _selectedLastRow = row + 1;
        } else {
            _selectedLastRow = row;
        }
        emit selectionChanged();
    }

    std::tuple<int, int> LogTableModel::getSelection() {
        return {_selectedFirstRow, _selectedLastRow};
    }

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

    void LogTableModel::showIndexedColumns() {
        _showIndexedColumns = true;
    }

    bool LogTableModel::isSelected(int row) {
        auto first = _selectedFirstRow;
        auto last = _selectedLastRow;
        if (_selectedLastRow < _selectedFirstRow) {
            std::swap(first, last);
            last++;
        }
        return first <= row && row < last;
    }

    uint64_t LogTableModel::lineOffset(uint64_t row) {
        return _index->mapIndex(row);
    }

    int LogTableModel::findRow(uint64_t lineOffset) {
        if (!_index)
            return lineOffset;
        auto rows = boost::irange(0, rowCount({}), 1);
        auto it = std::lower_bound(
            rows.begin(), rows.end(), lineOffset, [=](auto row, auto offset) {
                auto rowOffset = _index->mapIndex(row);
                return rowOffset < offset;
            });
        return it == rows.end() ? -1 : *it;
    }

    int LogTableModel::maxColumnWidth(int column) {
        return _columns[column].maxWidth;
    }

    void LogTableModel::setColumnWidths(std::vector<int> widths) {
        assert(widths.size() == _columns.size());
        for (auto i = 0u; i < widths.size(); ++i) {
            _columns[i].maxWidth = widths[i];
        }
    }

    QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (section == -1 || orientation != Qt::Horizontal)
            return QVariant();
        if (role == (int)HeaderDataRole::IsIndexed) {
            return _showIndexedColumns && _columns[section].indexed;
        }
        if (role == (int)HeaderDataRole::IsFilterActive) {
            return _columns[section].filterActive;
        }
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            if (section == (int)_columns.size() - 1)
                return {};
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
            return QString("%0").arg(lineIndex + 1);
        std::vector<std::string> line;
        _parser->readLine(lineIndex, line);
        size_t columnIndex = index.column() - Regular;
        if (columnIndex < line.size()) {
            return QString::fromStdString(line[columnIndex]);
        }
        if (index.column() == (int)_columns.size() - 1) {
            std::string rawLine;
            _parser->readLine(lineIndex, rawLine);
            return QString::fromStdString(rawLine);
        }
        return "";
    }

} // namespace gui
