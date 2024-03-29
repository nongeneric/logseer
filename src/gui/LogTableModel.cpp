#include "LogTableModel.h"

#include <QColor>
#include <gui/GraphemeMap.h>
#include <boost/range/irange.hpp>
#include <fmt/format.h>
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

bool LogTableModel::extendSelection(int row, int column, int index) {
    if (_selectedRow.left() == -1)
        return false;

    row = std::clamp(row, 0, rowCount({}) - 1);
    auto sameColumn = _selectedColumn == column;
    auto sameChar = _selectedChar.left() == index && _selectedChar.right() == index;

    auto rowChanged = _selectedRow.extend(row);
    bool charChanged = false;

    if (_selectedRow.size() == 1 && _selectedRow.original() == row) {
        if (sameColumn && sameChar)
            return false;
        if (column < _selectedColumn) {
            index = 0;
        } else if (column > _selectedColumn) {
            index = data(this->index(row, _selectedColumn), Qt::DisplayRole).toString().size() - 1;
        }
        charChanged = _selectedChar.extend(index);
    }
    _selectionExtended = true;

    if (!rowChanged && !charChanged)
        return false;

    emit selectionChanged();
    return true;
}

std::optional<RowSelection> LogTableModel::getRowSelection() const {
    if (_selectedRow.left() == -1)
        return {};
    return RowSelection{_selectedRow.left(), _selectedRow.right()};
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

std::vector<std::string> LogTableModel::values(int column) const {
    if (!_index)
        return {};
    std::vector<std::string> values;
    for (const auto& info : _index->getValues(column - 1)) {
        values.push_back(info.value);;
    }
    return values;
}

LogTableModel::LogTableModel(seer::FileParser* parser)
    : _parser(parser),
      _parserContext(_parser->lineParser()->createContext()) {
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
    GraphemeMap gmap(QString::fromStdString(str), nullptr);
    return gmap.graphemeSize();
}

void LogTableModel::copyLines(uint64_t begin, uint64_t end, LogTableModel::LineHandler accept) {
    std::vector<size_t> widths;
    widths.push_back(fmt::format("{}", lineOffset(end - 1) + 1).size());
    for (auto i = 1u; i < _columns.size(); ++i) {
        widths.push_back(_columns[i].name.size());
    }
    std::vector<std::string> columns;
    copyRawLines(begin, end, [&] (auto& line) {
        if (_parser->lineParser()->parseLine(line, columns, *_parser->lineParser()->createContext())) {
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
        if (_parser->lineParser()->parseLine(line, columns, *_parser->lineParser()->createContext())) {
            append(fmt::format("{}", lineOffset(index) + 1), 0);
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

    if (it == rows.end())
        return -1;

    if (_index->mapIndex(*it) != lineOffset)
        return -1;

    return *it;
}

void LogTableModel::setColumnWidths(std::vector<seer::ColumnWidth> widths) {
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
    if (role == (int)HeaderDataRole::IsAutosize) {
        return _columns[section].autosize;
    }
    if (role == (int)HeaderDataRole::LongestColumnIndex) {
        return rowCount({}) > 0 ? _columns[section].maxWidth.index : -1;
    }
    return QVariant();
}

int LogTableModel::rowCount([[maybe_unused]] const QModelIndex& parent) const {
    if (!_index)
        return _parser->lineCount();
    return _index->getLineCount();
}

int LogTableModel::unfilteredRowCount() const {
    return _parser->lineCount();
}

int LogTableModel::columnCount([[maybe_unused]] const QModelIndex& parent) const {
    return _columns.size();
}

QVariant LogTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return {};
    auto lineIndex = lineOffset(index.row());
    if (role != Qt::DisplayRole && role != Qt::ForegroundRole)
        return {};
    std::vector<std::string> line;
    readAndParseLine(*_parser, lineIndex, line, *_parserContext);
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
