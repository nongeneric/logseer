#include "Index.h"

#include "Log.h"
#include <numeric>

namespace seer {

    void Index::filter(const std::vector<ColumnFilter>& filters) {
        if (filters.empty()) {
            _filtered = false;
            return;
        }
        _filtered = true;

        log_info("started filtering");

        std::vector<ewah_bitset> perColumn;
        for (auto& filter : filters) {
            std::vector<const ewah_bitset*> perValue;
            assert(_columns[filter.column].indexed);
            auto& column = _columns[filter.column];
            for (auto& value : filter.selected) {
                perValue.push_back(&column.index[value]);
            }
            perColumn.push_back(fast_logicalor(perValue.size(), &perValue[0]));
        }

        _filter = std::accumulate(
            begin(perColumn) + 1, end(perColumn), perColumn[0], std::bit_and<>());

        log_info("filtering complete");

        _lineCount = 0;
        _lineMap.clear();
        for (auto index : _filter) {
            _lineMap.push_back(index);
            _lineCount++;
        }

        log_info("building lineMap complete");
    }

    uint64_t Index::getLineCount() {
        if (_filtered)
            return _lineCount;
        return _unfilteredLineCount;
    }

    uint64_t Index::mapIndex(uint64_t index) {
        if (!_filtered)
            return index;
        assert(index < _lineMap.size());
        return _lineMap[index];
    }

    void Index::index(FileParser* fileParser, ILineParser* lineParser) {
        log_info("started indexing");

        for (auto format : lineParser->getColumnFormats()) {
            _columns.push_back({{}, format.indexed});
        }

        fileParser->index([&](auto index, auto& line) {
            for (auto i = 0u; i < line.size(); ++i) {
                if (_columns[i].indexed) {
                    _columns[i].index[line[i]].set(index);
                }
            }
        });
        _unfilteredLineCount = fileParser->lineCount();

        log_info("indexing complete");
    }

    std::vector<std::string> Index::getValues(int column) {
        assert(_columns.at(column).indexed);
        std::vector<std::string> values;
        for (auto& pair : _columns[column].index) {
            values.push_back(pair.first);
        }
        return values;
    }

} // namespace seer
