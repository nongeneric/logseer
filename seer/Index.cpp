#include "Index.h"

#include "Log.h"
#include <tbb/concurrent_queue.h>
#include <tbb/parallel_for_each.h>
#include <numeric>
#include <optional>
#include <thread>
#include <tuple>

namespace seer {

    class Indexer {
        using Result = std::vector<ColumnInfo>;
        using QueueItem = std::optional<std::tuple<std::string, int>>;

        FileParser* _fileParser;
        ILineParser* _lineParser;
        std::function<bool()> _stopRequested;
        std::function<void(uint64_t, uint64_t)> _progress;
        std::vector<ColumnInfo>* _columns;
        tbb::concurrent_bounded_queue<QueueItem> _queue;
        unsigned _threadCount;
        std::vector<Result> _results;
        std::vector<std::thread> _threads;
        std::vector<ewah_bitset> _failures;

        void combine(auto& index, auto& failed, auto& result) {
            auto i = index.begin();
            auto f = failed.begin();
            auto ii = index.toArray();
            while (i != index.end()) {
                result.set(*i);
                while (f != failed.end() && *f < *i) {
                    ++f;
                }
                if (f == failed.end()) {
                    ++i;
                    while (i != index.end()) {
                        result.set(*i++);
                    }
                    break;
                }
                if (*i + 1 == *f) {
                    auto delta = 1;
                    while (f != failed.end() && delta == 1) {
                        result.set(*f);
                        auto pf = *f;
                        ++f;
                        delta = *f - pf;
                    }
                }
                ++i;
            }
        }

        void prepareThreads() {
            constexpr auto itemsPerThread = 256;
            _threadCount = std::thread::hardware_concurrency();
            _queue.set_capacity(itemsPerThread * _threadCount);

            Result emptyIndex;
            for (auto format : _lineParser->getColumnFormats()) {
                emptyIndex.push_back({{}, format.indexed});
            }

            _results = {_threadCount, emptyIndex};
            _threads.resize(_threadCount);
            _failures.resize(_threadCount);

            *_columns = emptyIndex;
        }

        void threadBody(int id) {
            std::vector<std::string> columns;
            int lastLineIndex = 0;
            QueueItem item;
            for (;;) {
                _queue.pop(item);
                if (!item)
                    return;
                auto& [line, lineIndex] = *item;
                assert(lineIndex >= lastLineIndex);
                lastLineIndex = lineIndex;
                if (_lineParser->parseLine(line, columns)) {
                    auto& index = _results[id];
                    for (auto i = 0u; i < columns.size(); ++i) {
                        if (index[i].indexed) {
                            index[i].index[columns[i]].set(lineIndex);
                        }
                    }
                } else {
                    _failures[id].set(lineIndex);
                }
            }
        }

        void startThreads() {
            int threadId = 0;
            for (auto& th : _threads) {
                th = std::thread([this, id = threadId] {
                    threadBody(id);
                });
                threadId++;
            }
        }

        void stopThreads() {
            for (auto i = 0u; i < _threadCount; ++i) {
                _queue.push({});
            }

            for (auto& th : _threads) {
                th.join();
            }
        }

        bool pushLinesToThreads() {
            std::vector<std::string> columns;
            std::string line;
            for (auto index = 0ul, count = _fileParser->lineCount(); index < count; ++index) {
                if (_stopRequested()) {
                    stopThreads();
                    return false;
                }
                _fileParser->readLine(index, line);
                _queue.push(std::tuple(line, index));
                if (_progress) {
                    _progress(index, count);
                }
            }
            return true;
        }

        void discoverMultilines() {
            std::vector<const ewah_bitset*> failurePointers;
            for (auto& failure : _failures) {
                failurePointers.push_back(&failure);
            }

            auto multilines = fast_logicalor(failurePointers.size(), &failurePointers[0]);
            std::vector<ewah_bitset*> allIndexes;
            for (auto tid = 0u; tid < _threadCount; ++tid) {
                auto& result = _results[tid];
                for (auto& column : result) {
                    for (auto& [_, index] : column.index) {
                        allIndexes.push_back(&index);
                    }
                }
            }

            tbb::parallel_for(size_t{}, allIndexes.size(), [&] (auto i) {
                ewah_bitset combined;
                auto index = allIndexes[i];
                combine(*index, multilines, combined);
                *index = combined;
            });
        }

        void reduceIndexes() {
            for (auto& result : _results) {
                assert(_columns->size() == result.size());
                for (auto i = 0u; i < _columns->size(); ++i) {
                    auto& column = (*_columns)[i];
                    assert(column.indexed == result[i].indexed);
                    for (auto& [name, set] : result[i].index) {
                        auto existing = column.index.find(name);
                        if (existing == end(column.index)) {
                            column.index[name] = set;
                        } else {
                            auto& existingSet = existing->second;
                            existingSet = existingSet | set;
                        }
                    }
                }
            }
        }

    public:
        Indexer(FileParser* fileParser,
                ILineParser* lineParser,
                std::function<bool()> stopRequested,
                std::function<void(uint64_t, uint64_t)> progress,
                std::vector<ColumnInfo>* columns)
            : _fileParser(fileParser),
              _lineParser(lineParser),
              _stopRequested(stopRequested),
              _progress(progress),
              _columns(columns) {}

        bool index() {
            auto columnFormats = _lineParser->getColumnFormats();

            if (std::find_if(begin(columnFormats), end(columnFormats), [](auto& format) {
                    return format.indexed;
                }) == end(columnFormats))
                return true;

            log_info("started indexing");

            prepareThreads();
            startThreads();

            if (!pushLinesToThreads())
                return false;

            stopThreads();

            log_info("indexing multilines");

            discoverMultilines();

            log_info("consolidating indexes");

            reduceIndexes();

            log_info("indexing complete");

            return true;
        }
    };

    Index::Index(uint64_t unfilteredLineCount)
        : _lineMap(1024), _unfilteredLineCount(unfilteredLineCount) {}

    void Index::filter(const std::vector<ColumnFilter>& filters) {
        _filters = filters;

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
        _unfilteredLineCount = fileParser->lineCount();
        Indexer indexer(fileParser, lineParser, stopRequested, progress, &_columns);
        return indexer.index();
    }

    std::vector<ColumnIndexInfo> Index::getValues(int column) {
        assert(_columns.at(column).indexed);
        std::vector<ColumnIndexInfo> values;
        for (auto& [value, index] : _columns[column].index) {
            auto checked = true;
            auto filter = std::find_if(begin(_filters), end(_filters), [=](auto& f) {
                return f.column == column;
            });
            if (filter != end(_filters)) {
                auto& selected = filter->selected;
                checked = selected.find(value) != end(selected);
            }
            values.push_back({value, checked, index.numberOfOnes()});
        }
        return values;
    }

} // namespace seer
