#include "Index.h"

#include "Log.h"
#include <tbb/concurrent_queue.h>
#include <numeric>
#include <optional>
#include <thread>
#include <tuple>

namespace seer {

    Index::Index(uint64_t unfilteredLineCount)
        : _lineMap(1024), _unfilteredLineCount(unfilteredLineCount) {}

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

        _lineMap.clear();
        for (auto index : _filter) {
            _lineMap.add(index);
        }

        log_info("building lineMap complete");
    }

    void Index::search(FileParser* fileParser, std::string text, bool caseSensitive) {
        _lineMap.clear();
        std::string line;
        auto pred = caseSensitive
                ? [](char a, char b) { return a == b; }
                : [](char a, char b) { return std::toupper(a) == std::toupper(b); };
        auto add = [&] (auto index) {
            fileParser->readLine(index, line);
            auto it = std::search(begin(line), end(line), begin(text), end(text), pred);
            if (it != end(line)) {
                _lineMap.add(index);
            }
        };
        if (_filtered) {
            for (auto index : _filter) {
                add(index);
            }
        } else {
            for (uint64_t i = 0; i < _unfilteredLineCount; ++i) {
                add(i);
            }
            _filtered = true;
        }
    }

    uint64_t Index::getLineCount() {
        if (_filtered)
            return _lineMap.size();
        return _unfilteredLineCount;
    }

    uint64_t Index::mapIndex(uint64_t index) {
        if (!_filtered)
            return index;
        return _lineMap.get(index);
    }

    bool Index::index(FileParser* fileParser,
                      ILineParser* lineParser,
                      std::function<bool()> stopRequested,
                      std::function<void(uint64_t, uint64_t)> progress)
    {
        log_info("started indexing");

        using Result = std::vector<ColumnInfo>;
        using QueueItem = std::optional<std::tuple<std::string, int>>;

        tbb::concurrent_bounded_queue<QueueItem> queue;
        constexpr auto itemsPerThread = 256;
        auto threadCount = std::thread::hardware_concurrency();
        queue.set_capacity(itemsPerThread * threadCount);

        Result emptyIndex;
        for (auto format : lineParser->getColumnFormats()) {
            emptyIndex.push_back({{}, format.indexed});
        }

        std::vector<Result> results(threadCount, emptyIndex);
        std::vector<std::thread> threads(threadCount);
        std::vector<ewah_bitset> failures(threadCount);
        int threadId = 0;
        for (auto& th : threads) {
            th = std::thread([&, id = threadId] {
                std::vector<std::string> columns;
                int lastLineIndex = 0;
                QueueItem item;
                for (;;) {
                    queue.pop(item);
                    if (!item)
                        return;
                    auto& [line, lineIndex] = *item;
                    assert(lineIndex >= lastLineIndex);
                    lastLineIndex = lineIndex;
                    if (lineParser->parseLine(line, columns)) {
                        auto& index = results[id];
                        for (auto i = 0u; i < columns.size(); ++i) {
                            if (index[i].indexed) {
                                index[i].index[columns[i]].set(lineIndex);
                            }
                        }
                    } else {
                        failures[id].set(lineIndex);
                    }
                }
            });
            threadId++;
        }

        auto stopThreads = [&] {
            for (auto i = 0u; i < threadCount; ++i) {
                queue.push({});
            }

            for (auto& th : threads) {
                th.join();
            }
        };

        std::vector<std::string> columns;
        std::string line;
        for (auto index = 0ul, count = fileParser->lineCount(); index < count; ++index) {
            if (stopRequested()) {
                stopThreads();
                return false;
            }
            fileParser->readLine(index, line);
            queue.push(std::tuple(line, index));
            if (progress) {
                progress(index, count);
            }
        }

        stopThreads();

        log_info("indexing multilines");

        std::vector<const ewah_bitset*> failurePointers;
        for (auto& failure : failures) {
            failurePointers.push_back(&failure);
        }
        _multilines = fast_logicalor(failurePointers.size(), &failurePointers[0]);

        auto lineCount = fileParser->lineCount();
        for (auto tid = 0u; tid < threadCount; ++tid) {
            auto& result = results[tid];
            for (auto& column : result) {
                for (auto& [_, index] : column.index) {
                    ewah_bitset multilines;
                    for (auto val : _multilines) {
                        if (val == 0)
                            continue;
                        if (index.get(val - 1)) {
                            auto i = val;
                            while (i < lineCount && _multilines.get(val)) {
                                index.set(val);
                                i++;
                            }
                        }
                    }
                    index = index | multilines;
                }
            }
        }

        log_info("consolidating indexes");

        _columns = emptyIndex;
        for (auto& result : results) {
            assert(_columns.size() == result.size());
            for (auto i = 0u; i < _columns.size(); ++i) {
                assert(_columns[i].indexed == result[i].indexed);
                for (auto& [name, set] : result[i].index) {
                    auto existing = _columns[i].index.find(name);
                    if (existing == end(_columns[i].index)) {
                        _columns[i].index[name] = set;
                    } else {
                        auto& existingSet = existing->second;
                        existingSet = existingSet | set;
                    }
                }
            }
        }

        _unfilteredLineCount = fileParser->lineCount();

        log_info("indexing complete");

        return true;
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
