#include "LogTableModel.h"

#include <QColor>
#include <seer/bformat.h>
#include <gui/GraphemeMap.h>
#include <boost/range/irange.hpp>
#include <numeric>

namespace gui {

enum ColumnType {
    LineNumber = 0,
    Regular = 1
};

void LogTableModel::setSelection(int row, int column, int index) {
    _selectedRow.set(row);
    _selectedChar.set(index);
    _selectedColumn = column;
    _selectionExtended = false;
    emit selectionChanged();
}

void LogTableModel::forceColumnSelection() {
    _selectionExtended = true;
}

void LogTableModel::extendSelection(int row, int column, int index) {
    if (_selectedRow.left() == -1)
        return;

    row = std::clamp(row, 0, rowCount({}) - 1);
    auto sameColumn = _selectedColumn == column;
    auto sameChar = _selectedChar.left() == index && _selectedChar.right() == index;

    _selectedRow.extend(row);

    if (_selectedRow.size() == 1 && _selectedRow.original() == row) {
        if (sameColumn && sameChar)
            return;
        if (column < _selectedColumn) {
            index = 0;
        } else if (column > _selectedColumn) {
            index = data(this->index(row, _selectedColumn), Qt::DisplayRole).toString().size() - 1;
        }
        _selectedChar.extend(index);
    }
    _selectionExtended = true;
    emit selectionChanged();
}

std::tuple<int, int> LogTableModel::getRowSelection() const {
    return {_selectedRow.left(), _selectedRow.right()};
}

LogTableSelection LogTableModel::getSelection() const {
    if (_selectedRow.left() == -1)
        return std::monostate();
    if (_selectionExtended && _selectedRow.size() == 1) {
        return ColumnSelection{
            _selectedRow.left(), _selectedColumn, _selectedChar.left(), _selectedChar.right()};
    }
    return RowSelection{_selectedRow.left(), _selectedRow.right()};
}

LogTableModel::LogTableModel(seer::FileParser* parser)
    : _parser(parser) {
    _columns.push_back({"#", false, true});
    for (auto& column : parser->lineParser()->getColumnFormats()) {
        _columns.push_back(
            {QString::fromStdString(column.header), column.indexed, column.autosize});
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

void LogTableModel::copyRawLines(uint64_t begin, uint64_t end, LogTableModel::LineHandler accept) {
    std::string line;
    for (auto i = begin; i != end; ++i) {
        _parser->readLine(lineOffset(i), line);
        accept(line);
    }
}

size_t graphemeLength(const std::string& str) {
    GraphemeMap gmap(QString::fromStdString(str));
    return gmap.graphemeSize();
}

void LogTableModel::copyLines(uint64_t begin, uint64_t end, LogTableModel::LineHandler accept) {
    std::vector<size_t> widths;
    widths.push_back(bformat("%d", lineOffset(end) + 1).size());
    for (auto i = 1u; i < _columns.size(); ++i) {
        widths.push_back(_columns[i].name.size());
    }
    std::vector<std::string> columns;
    copyRawLines(begin, end, [&] (auto& line) {
        if (_parser->lineParser()->parseLine(line, columns)) {
            for (auto i = 0u; i < columns.size(); ++i) {
                auto& width = widths.at(i + 1);
                width = std::max(width, graphemeLength(columns[i]));
            }
        } else {
            widths.back() = std::max(widths.back(), graphemeLength(line));
        }
    });

    std::string formatted;

    auto appendSpaces = [&] (auto count, char ch = ' ') {
        for (auto i = 0u; i < count; ++i) {
            formatted += ch;
        }
    };

    const auto spacing = 3;
    auto append = [&](const auto& column, auto index) {
        auto isMessage = static_cast<size_t>(index) == _columns.size() - 1;
        auto actualSpacing = isMessage ? 0u : spacing;
        auto width = widths.at(index) + actualSpacing;
        formatted += column;
        if (!isMessage) {
            appendSpaces(width - graphemeLength(column));
        }
    };

    for (auto i = 0u; i < _columns.size(); ++i) {
        append(_columns[i].name.toStdString(), i);
    }
    accept(formatted);

    formatted.clear();
    auto separatorWidth = std::accumulate(widths.begin(), widths.end(), 0u);
    separatorWidth += (widths.size() - 1) * spacing;
    appendSpaces(separatorWidth, '-');
    accept(formatted);

    auto index = begin;
    copyRawLines(begin, end, [&] (auto& line) {
        formatted.clear();
        if (_parser->lineParser()->parseLine(line, columns)) {
            append(bformat("%d", lineOffset(index) + 1), 0);
            for (auto i = 0u; i < columns.size(); ++i) {
                append(columns[i], i + 1);
            }
        } else {
            auto width = std::accumulate(widths.begin(), widths.end() - 1, 0u);
            appendSpaces(width + (widths.size() - 1) * spacing);
            append(line, widths.size() - 1);
        }
        accept(formatted);
        index++;
    });
}

uint64_t LogTableModel::lineOffset(uint64_t row) const {
    return _index ? _index->mapIndex(row) : row;
}

int LogTableModel::findRow(uint64_t lineOffset) {
    if (!_index)
        return lineOffset;
    auto rows = boost::irange(0, rowCount({}), 1);
    auto it = std::lower_bound(
        rows.begin(), rows.end(), lineOffset, [this](auto row, auto offset) {
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
    emit columnWidthsChanged();
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
    if (role == (int)HeaderDataRole::Autosize) {
        return _columns[section].autosize ? _columns[section].maxWidth : -1;
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
    auto lineIndex = lineOffset(index.row());
    if (role != Qt::DisplayRole && role != Qt::ForegroundRole)
        return {};
    std::vector<std::string> line;
    _parser->readLine(lineIndex, line);
    if (role == Qt::ForegroundRole)
        return QColor(_parser->lineParser()->rgb(line));
    if (index.column() == LineNumber)
        return QString("%0").arg(lineIndex + 1);
    size_t columnIndex = index.column() - Regular;
    if (columnIndex < line.size())
        return QString::fromStdString(line[columnIndex]);
    if (index.column() == (int)_columns.size() - 1) {
        std::string rawLine;
        _parser->readLine(lineIndex, rawLine);
        return QString::fromStdString(rawLine);
    }
    return "";
}

} // namespace gui
