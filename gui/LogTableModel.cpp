#include "LogTableModel.h"

#include <QColor>
#include <QTextBoundaryFinder>
#include <seer/bformat.h>
#include <boost/range/irange.hpp>
#include <numeric>

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
        if (_selectedFirstRow == -1)
            return;
        row = std::max(row, 0);
        row = std::min(row, rowCount({}) - 1);
        _selectedLastRow = row + 1;
        if (_selectedFirstRow < _selectedLastRow) {
            _selectedLastRow = row + 1;
        } else {
            _selectedLastRow = row;
        }
        emit selectionChanged();
    }

    std::tuple<int, int> LogTableModel::getSelection() {
        auto first = _selectedFirstRow;
        auto last = _selectedLastRow;
        if (_selectedLastRow < _selectedFirstRow) {
            std::swap(first, last);
            last++;
        }
        return {first, last};
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
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, QString::fromStdString(str));
        size_t length = 0;
        while (finder.toNextBoundary() != -1)
            length++;
        return length;
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

    bool LogTableModel::isSelected(int row) {
        auto [first, last] = getSelection();
        return first <= row && row < last;
    }

    uint64_t LogTableModel::lineOffset(uint64_t row) const {
        return _index ? _index->mapIndex(row) : row;
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
