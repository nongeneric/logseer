#include "Index.h"

#include "Log.h"
#include "ParallelFor.h"
#include "SPMCQueue.h"
#include "Searcher.h"
#include <QString>
#include <numeric>
#include <optional>
#include <thread>
#include <tuple>
#include <algorithm>

namespace seer {

constexpr float g_maxFailureRatio = .1f;
constexpr int g_producerBatchSize = 1000;
constexpr int g_consumerBatchSize = 200;
constexpr int g_stringPoolSize = g_producerBatchSize;

int lineLength(const std::string& line) {
    return line.size();
}

template <class T>
class Pool {
    moodycamel::ConcurrentQueue<T> _queue;
    moodycamel::ConsumerToken _consumerToken{_queue};

public:
    void enqueue(moodycamel::ProducerToken& token, T* items, int size) {
        _queue.enqueue_bulk(token, std::make_move_iterator(items), size);
    }

    int dequeue(T* items, int maxSize) {
        int size = 0;
        while (size = _queue.try_dequeue_bulk(_consumerToken, items, maxSize), !size) {
            std::this_thread::yield();
        }
        return size;
    }

    template <class F>
    void preallocate(int size, F create) {
        while (size--) {
            _queue.enqueue(create());
        }
    }

    moodycamel::ProducerToken getToken() {
        return moodycamel::ProducerToken(_queue);
    }
};

class Indexer {
    struct QueueItem {
        std::string line;
        int lineNumber;
    };

    using Result = std::vector<ColumnInfo>;
    using QueueItemPtr = std::unique_ptr<QueueItem>;

    FileParser* _fileParser;
    ILineParser* _lineParser;
    unsigned _maxThreads;
    std::function<bool()> _stopRequested;
    std::function<void(uint64_t, uint64_t)> _progress;
    std::vector<ColumnInfo>* _columns;
    SPMCQueue<QueueItemPtr> _queue;
    Pool<QueueItemPtr> _stringPool;
    std::vector<Result> _results;
    std::vector<std::thread> _threads;
    std::vector<ewah_bitset> _failures;
    ewah_bitset _combinedFailures;

    void prepareThreads() {
        auto threadCount = std::thread::hardware_concurrency();
        if (_maxThreads) {
            threadCount = std::min(_maxThreads, threadCount);
        }

        Result emptyIndex;
        for (auto format : _lineParser->getColumnFormats()) {
            emptyIndex.push_back({{}, format.indexed, {}, {}});
        }

        _results = {threadCount, emptyIndex};
        _threads.resize(threadCount);
        _failures.resize(threadCount);

        *_columns = emptyIndex;
    }

    void threadBody(int id, ILineParserContext& lineParserContext) {
        std::vector<std::string> columns;
        [[maybe_unused]] int lastLineIndex = 0;
        auto lastColumn = _lineParser->getColumnFormats().size() - 1;
        std::vector<QueueItemPtr> items(g_consumerBatchSize);
        auto token = _stringPool.getToken();
        for (;;) {
            int size = _queue.dequeue(&items[0], items.size());
            if (!size)
                return;

            for (auto i = 0; i < size; ++i) {
                auto& [line, lineIndex] = *items[i];
                assert(lineIndex >= lastLineIndex);
                lastLineIndex = lineIndex;
                auto& index = _results[id];
                if (_lineParser->parseLine(line, columns, lineParserContext)) {
                    for (auto i = 0u; i < columns.size(); ++i) {
                        index[i].maxWidth = std::max(index[i].maxWidth, {lineIndex, lineLength(columns[i])});
                        if (index[i].indexed) {
                            index[i].index[columns[i]].set(lineIndex);
                        }
                    }
                } else {
                    _failures[id].set(lineIndex);
                    auto& info = index[lastColumn];
                    info.maxWidth = std::max(info.maxWidth, {lineIndex, lineLength(line)});
                }
            }
            _stringPool.enqueue(token, &items[0], size);
        }
    }

    void startThreads() {
        int threadId = 0;
        for (auto& th : _threads) {
            th = std::thread([=, this] {
                auto context = _lineParser->createContext();
                threadBody(threadId, *context);
            });
            threadId++;
        }
    }

    void stopThreads() {
        _queue.stop();

        for (auto& th : _threads) {
            th.join();
        }
    }

    bool pushLinesToThreads(bool parsingOnly) {
        std::vector<std::string> columns;
        std::string line;
        std::vector<QueueItemPtr> items;
        int lineNumber = 0;
        size_t itemsPos = 0;
        _stringPool.preallocate(g_stringPoolSize, [] { return std::make_unique<QueueItem>(); });
        _fileParser->index([&] (uint64_t pos, uint64_t fileSize) {
            if (_progress) {
                _progress(pos, fileSize);
            }
        }, [&] (const auto& line) {
            if (parsingOnly)
                return;
            if (itemsPos == items.size()) {
                _queue.enqueue(&items[0], items.size());
                items.resize(g_producerBatchSize);
                items.resize(_stringPool.dequeue(&items[0], g_producerBatchSize));
                itemsPos = 0;
            }
            items[itemsPos]->line = line;
            items[itemsPos++]->lineNumber = lineNumber++;
        }, [&] {
            return _stopRequested();
        });
        if (_stopRequested()) {
            stopThreads();
            return false;
        }

        if (!items.empty()) {
            _queue.enqueue(&items[0], itemsPos);
        }

        return true;
    }

    void discoverMultilines() {
        std::vector<const ewah_bitset*> failurePointers;
        for (auto& failure : _failures) {
            failurePointers.push_back(&failure);
        }

        _combinedFailures = fast_logicalor(failurePointers.size(), &failurePointers[0]);

        Result columnInfos;
        for (auto format : _lineParser->getColumnFormats()) {
            columnInfos.push_back({{}, format.indexed, {}, {}});
        }
        std::vector<std::string> columns;
        auto failures = _combinedFailures.toArray();

        auto failureRatio = failures.size() / static_cast<float>(_fileParser->lineCount());
        auto readConsequently = failureRatio > g_maxFailureRatio;

        auto lineParserContext = _lineParser->createContext();

        size_t pos = -1;
        std::string line;
        auto readLine = [&] (auto index) {
            std::string text;
            if (!readConsequently) {
                readAndParseLine(*_fileParser, index, columns, *lineParserContext);
                return;
            }

            if (pos == -1ull) {
                pos = index;
            }
            while (pos != index) {
                _fileParser->readLine(pos, line);
                pos++;
            }
            [[maybe_unused]] auto result = readAndParseLine(*_fileParser, index, columns, *lineParserContext);
            assert(result);
            pos++;
        };

        for (size_t i = 0; i < failures.size();) {
            auto failureIndex = failures[i];
            if (failureIndex == 0) {
                columns.clear();
                columns.resize(columnInfos.size());
            } else {
                readLine(failureIndex - 1);
            }

            auto firstFailure = i;
            while (i < failures.size() && failures[i] + 1 == failures[i + 1]) {
                i++;
            }
            i++;

            assert(_columns->size() == columnInfos.size());
            for (size_t c = 0; c < _columns->size(); ++c) {
                if (!columnInfos[c].indexed)
                    continue;
                const auto& value = columns[c];
                auto& bitmap = columnInfos[c].index[value];
                for (auto j = firstFailure; j < i; ++j) {
                    bitmap.set(failures[j]);
                }
            }
        }

        _results.clear();
        _results.push_back(columnInfos);
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
                        column.index[name] = std::move(set);
                    } else {
                        auto& existingSet = existing->second;
                        existingSet = existingSet | set;
                    }
                }
            }
            result.clear();
        }
    }

    void logIndexSize() {
        size_t totalSize = 0;
        size_t count = 0;
        for (auto& column : *_columns) {
            for (auto& [_, set] : column.index) {
                totalSize += set.sizeOnDisk();
            }
            count += column.index.size();
        }

        log_infof("file index: %g MB",
                  static_cast<double>(_fileParser->calcLineIndexSize()) / (1 << 20));
        log_infof("bitsets: %d, total size: %g MB",
                  count,
                  static_cast<double>(totalSize) / (1 << 20));
    }

public:
    Indexer(FileParser* fileParser,
            ILineParser* lineParser,
            unsigned maxThreads,
            std::function<bool()> stopRequested,
            std::function<void(uint64_t, uint64_t)> progress,
            std::vector<ColumnInfo>* columns)
        : _fileParser(fileParser),
          _lineParser(lineParser),
          _maxThreads(maxThreads),
          _stopRequested(stopRequested),
          _progress(progress),
          _columns(columns) {}

    bool index() {
        auto columnFormats = _lineParser->getColumnFormats();

        if (columnFormats.size() == 1) {
            log_infof("parser [%s] has a single column and doesn't require indexing", _lineParser->name());
            _columns->clear();
            _columns->resize(1);
            log_info("started parsing");
            if (!pushLinesToThreads(true))
                return false;
            log_info("parsing done");
            return true;
        }

        prepareThreads();

        auto past = std::chrono::high_resolution_clock::now();

        log_info("started indexing");

        startThreads();

        if (!pushLinesToThreads(false))
            return false;

        stopThreads();

        log_info("consolidating indexes");

        reduceIndexes();

        log_info("indexing multilines");

        discoverMultilines();
        reduceIndexes();

        logIndexSize();

        auto now = std::chrono::high_resolution_clock::now();
        log_infof("indexing done in %d ms",
                  std::chrono::duration_cast<std::chrono::milliseconds>(now - past).count());

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

    auto lineParserContext = fileParser->lineParser()->createContext();

    auto add = [&] (auto index, auto histIndex, auto histSize) {
        fileParser->readLine(index, line);
        auto lineToSearch = &line;
        if (messageOnly) {
            columns.clear();
            fileParser->lineParser()->parseLine(line, columns, *lineParserContext);
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
                  unsigned maxThreads,
                  std::function<bool()> stopRequested,
                  std::function<void(uint64_t, uint64_t)> progress)
{
    Indexer indexer(fileParser, lineParser, maxThreads, stopRequested, progress, &_columns);
    auto res = indexer.index();
    _unfilteredLineCount = fileParser->lineCount();
    return res;
}

std::vector<ColumnIndexInfo> Index::getValues(int column) {
    assert(_columns.at(column).indexed);
    std::vector<ColumnIndexInfo> values;
    for (auto& [value, index] : _columns[column].index) {
        auto checked = true;
        auto filter = std::find_if(begin(_filters), end(_filters), [&] (auto& filter) {
            return filter.column == column;
        });
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

    std::sort(begin(values), end(values), [&](auto& a, auto& b) {
        return a.value < b.value;
    });

    return values;
}

ColumnWidth Index::maxWidth(int column) {
    return _columns.at(column).maxWidth;
}

} // namespace seer
