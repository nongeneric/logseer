#include "Index.h"

#include "Log.h"
#include "ParallelFor.h"
#include "BoundedConcurrentQueue.h"
#include "Searcher.h"
#include <numeric>
#include <optional>
#include <thread>
#include <tuple>
#include <QString>

#include <range/v3/algorithm.hpp>

namespace seer {

int lineLength(const std::string& line) {
    return line.size() +
           (g_tabWidth - 1) * ranges::count_if(line, [](auto& ch) { return ch == '\t'; });
}

class Indexer {
    using Result = std::vector<ColumnInfo>;
    using QueueItem = std::optional<std::tuple<std::string, int>>;

    FileParser* _fileParser;
    ILineParser* _lineParser;
    std::function<bool()> _stopRequested;
    std::function<void(uint64_t, uint64_t)> _progress;
    std::vector<ColumnInfo>* _columns;
    BoundedConcurrentQueue<QueueItem> _queue;
    unsigned _threadCount;
    std::vector<Result> _results;
    std::vector<std::thread> _threads;
    std::vector<ewah_bitset> _failures;
    ewah_bitset _combinedFailures;

    void combine(const ewah_bitset& index, const ewah_bitset& failed, ewah_bitset& result) {
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
        auto capacity = itemsPerThread * _threadCount;
        _queue = BoundedConcurrentQueue<QueueItem>(capacity);

        Result emptyIndex;
        for (auto format : _lineParser->getColumnFormats()) {
            emptyIndex.push_back({{}, format.indexed, {}, {}});
        }

        _results = {_threadCount, emptyIndex};
        _threads.resize(_threadCount);
        _failures.resize(_threadCount);

        *_columns = emptyIndex;
    }

    void threadBody(int id) {
        std::vector<std::string> columns;
        [[maybe_unused]] int lastLineIndex = 0;
        auto lastColumn = _lineParser->getColumnFormats().size() - 1;
        QueueItem item;
        for (;;) {
            _queue.dequeue(item);
            if (!item)
                return;
            auto& [line, lineIndex] = *item;
            assert(lineIndex >= lastLineIndex);
            lastLineIndex = lineIndex;
            auto& index = _results[id];
            if (_lineParser->parseLine(line, columns)) {
                for (auto i = 0u; i < columns.size(); ++i) {
                    index[i].maxWidth = std::max(index[i].maxWidth, lineLength(columns[i]));
                    if (index[i].indexed) {
                        index[i].index[columns[i]].set(lineIndex);
                    }
                }
            } else {
                _failures[id].set(lineIndex);
                auto& info = index[lastColumn];
                info.maxWidth = std::max(info.maxWidth, lineLength(line));
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
            _queue.enqueue({});
        }

        for (auto& th : _threads) {
            th.join();
        }
    }

    bool pushLinesToThreads() {
        std::vector<std::string> columns;
        std::string line;
        for (uint64_t index = 0, count = _fileParser->lineCount(); index < count; ++index) {
            if (_stopRequested()) {
                stopThreads();
                return false;
            }
            _fileParser->readLine(index, line);
            _queue.enqueue(std::tuple(line, index));
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

        _combinedFailures = fast_logicalor(failurePointers.size(), &failurePointers[0]);
        std::vector<ewah_bitset*> allIndexes;
        for (auto tid = 0u; tid < _threadCount; ++tid) {
            auto& result = _results[tid];
            for (auto& column : result) {
                for (auto& [_, index] : column.index) {
                    allIndexes.push_back(&index);
                }
            }
        }

        parallelFor(allIndexes, [this](auto index) {
            ewah_bitset combined;
            combine(*index, _combinedFailures, combined);
            *index = combined;
        });
    }

    void reduceIndexes() {
        for (auto& result : _results) {
            assert(_columns->size() == result.size());
            for (auto i = 0u; i < _columns->size(); ++i) {
                auto& column = (*_columns)[i];
                assert(column.indexed == result[i].indexed);
                column.maxWidth = std::max(column.maxWidth, result[i].maxWidth);
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

    void patchFailedStartingLines() {
        ewah_bitset set;
        auto i = 0u;
        for (auto value : _combinedFailures) {
            if (value != i)
                break;
            set.set(i);
            ++i;
        }

        if (set.numberOfOnes() == 0)
            return;

        for (auto& column : *_columns) {
            if (auto [it, inserted] = column.index.insert({"", set}); !inserted) {
                it->second = it->second | set;
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

        prepareThreads();

        log_info("started indexing");

        startThreads();

        if (!pushLinesToThreads())
            return false;

        stopThreads();

        log_info("indexing multilines");

        discoverMultilines();

        log_info("consolidating indexes");

        reduceIndexes();
        patchFailedStartingLines();

        log_info("indexing complete");

        return true;
    }
};

void Index::makePerColumnIndex(std::vector<ColumnFilter>::const_iterator first,
                               std::vector<ColumnFilter>::const_iterator last) {
    for (auto filter = first; filter != last; ++filter) {
        std::vector<const ewah_bitset*> perValue;
        assert(_columns[filter->column].indexed);
        auto& column = _columns[filter->column];
        for (auto& value : filter->selected) {
            perValue.push_back(&column.index[value]);
        }
        _columns[filter->column].combinedIndex = fast_logicalor(perValue.size(), &perValue[0]);
    }
}

Index::Index(uint64_t unfilteredLineCount)
    : _unfilteredLineCount(unfilteredLineCount) {}

void Index::filter(const std::vector<ColumnFilter>& filters) {
    _filters = filters;

    if (filters.empty()) {
        _filtered = false;
        return;
    }

    _filtered = true;

    log_info("started filtering");

    makePerColumnIndex(begin(filters), end(filters));
    _filter = _columns[_filters[0].column].combinedIndex;
    for (auto it = begin(_filters) + 1; it != end(_filters); ++it) {
        _filter = _filter & _columns[it->column].combinedIndex;
    }

    log_info("filtering complete");

    auto iewah = std::make_shared<IndexedEwah>(1024);
    iewah->init(_filter);
    _lineMap = std::move(iewah);

    log_info("building lineMap complete");
}

void Index::search(FileParser* fileParser,
                   std::string text,
                   bool regex,
                   bool caseSensitive,
                   bool messageOnly,
                   Hist& hist,
                   std::function<bool()> stopRequested,
                   std::function<void(uint64_t, uint64_t)> progress)
{
    std::string line;
    auto searcher = createSearcher(QString::fromStdString(text), regex, caseSensitive);
    auto lineMap = std::make_shared<RandomBitArray>(1024);
    std::vector<std::string> columns;
    auto add = [&] (auto index, auto histIndex, auto histSize) {
        fileParser->readLine(index, line);
        auto lineToSearch = &line;
        if (messageOnly) {
            columns.clear();
            fileParser->lineParser()->parseLine(line, columns);
            if (!columns.empty()) {
                lineToSearch = &columns.back();
            }
        }
        if (std::get<0>(searcher->search(QString::fromStdString(*lineToSearch), 0)) != -1) {
            lineMap->add(index);
            hist.add(histIndex, histSize);
        }
    };

    std::shared_ptr<int> guard(nullptr, [&](auto) {
        hist.freeze();
        _lineMap = lineMap;
    });

    if (_filtered) {
        auto size = _filter.numberOfOnes();
        uint64_t done = 0;
        for (auto index : _filter) {
            if (stopRequested())
                return;
            add(index, done, size);
            done++;
            if (progress)
                progress(index, _unfilteredLineCount);
        }
    } else {
        for (uint64_t i = 0; i < _unfilteredLineCount; ++i) {
            if (stopRequested())
                return;
            add(i, i, _unfilteredLineCount);
            if (progress)
                progress(i, _unfilteredLineCount);
        }
        _filtered = true;
    }
}

uint64_t Index::getLineCount() {
    if (_filtered)
        return _lineMap->size();
    return _unfilteredLineCount;
}

uint64_t Index::mapIndex(uint64_t index) {
    if (!_filtered)
        return index;
    return _lineMap->get(index);
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
        auto filter = ranges::find(_filters, column, &ColumnFilter::column);
        if (filter != end(_filters)) {
            auto& selected = filter->selected;
            checked = selected.find(value) != end(selected);
        }

        auto first = begin(_filters);
        auto last = end(_filters);
        if (filter != end(_filters)) {
            std::swap(*filter, end(_filters)[-1]);
            --last;
        }

        size_t count = 0;
        if (first == last) {
            count = index.numberOfOnes();
        } else {
            auto otherColumnsIndex = _columns[first->column].combinedIndex;
            for (auto it = first + 1; it != last; ++it) {
                otherColumnsIndex = otherColumnsIndex & _columns[it->column].combinedIndex;
            }
            count = (otherColumnsIndex & index).numberOfOnes();
        }
        values.push_back({value, checked, count});
    }

    ranges::sort(values, std::less<>(), &ColumnIndexInfo::value);

    return values;
}

int Index::maxWidth(int column) {
    return _columns.at(column).maxWidth;
}

} // namespace seer
