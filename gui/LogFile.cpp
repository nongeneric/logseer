#include "LogFile.h"
#include <range/v3/view.hpp>
#include <range/v3/action.hpp>
#include <range/v3/algorithm.hpp>

using namespace seer::task;
using namespace ranges;

namespace gui {

LogFile::LogFile(std::unique_ptr<std::istream>&& stream,
                 std::shared_ptr<seer::ILineParser> lineParser)
    : _lineParser(lineParser),
      _stream(std::move(stream)),
      _sm(static_cast<IStateHandler*>(this), _smLogger) {}

void LogFile::enterParsing() {
    emit stateChanged();
    _fileParser.reset(new seer::FileParser(_stream.get(), _lineParser.get()));
    _parsingTask.reset(createParsingTask(_fileParser.get()));
    _parsingTask->setStateChanged([=](auto state) {
        assert(state != TaskState::Failed);
        if (state == TaskState::Finished) {
            _dispatcher.postToUIThread([this] { index(); });
        }
    });
    _parsingTask->setProgressChanged([this](auto progress) {
        _dispatcher.postToUIThread([=] { emit progressChanged(progress); });
    });
    _parsingTask->start();
}

void LogFile::interruptParsing() {
    emit stateChanged();
    _parsingTask->stop();
}

void LogFile::enterIndexing() {
    emit stateChanged();
    _index.reset(new seer::Index(_fileParser->lineCount()));
    _indexingTask.reset(
        createIndexingTask(_index.get(), _fileParser.get(), _lineParser.get()));
    _indexingTask->setStateChanged([this](auto state) {
        assert(state != TaskState::Failed);
        if (state == TaskState::Finished) {
            _dispatcher.postToUIThread([this] { finish(); });
        } else if (state == TaskState::Paused) {
            _dispatcher.postToUIThread([this] { paused(); });
        } else if (state == TaskState::Stopped) {
            _dispatcher.postToUIThread([this] { finish(); });
        }
    });
    _indexingTask->setProgressChanged([this](auto progress) {
        _dispatcher.postToUIThread([=] { emit progressChanged(progress); });
    });
    _indexingTask->start();
}

void LogFile::interruptIndexing() {
    emit stateChanged();
    _indexingTask->stop();
}

void LogFile::doneInterrupted() {
    emit stateChanged();
}

void LogFile::pauseAndSearch(sm::SearchEvent event) {
    emit stateChanged();
    _scheduledSearchEvent = event;
    _indexingTask->pause();
}

void LogFile::resumeIndexing() {
    if (_indexingComplete) {
        finish();
    } else {
        emit stateChanged();
        _indexingTask->start();
    }
}

void LogFile::searchFromComplete(sm::SearchEvent event) {
    emit stateChanged();
    _searchingTask.reset(new SearchingTask(_fileParser.get(),
                                           _index.get(),
                                           event.text,
                                           event.regex,
                                           event.caseSensitive,
                                           event.messageOnly));
    _searchingTask->setStateChanged([this](auto state) {
        assert(state != TaskState::Failed);
        if (state == TaskState::Finished) {
            _dispatcher.postToUIThread([=] {
                auto newSearchTableModel = new LogTableModel(_fileParser.get());
                subscribeToSelectionChanged(_searchLogTableModel.get(), newSearchTableModel);
                _searchLogTableModel.reset(newSearchTableModel);
                _searchHist = _searchingTask->hist();
                _searchIndex = _searchingTask->index();
                _searchLogTableModel->setIndex(_searchIndex.get());
                finish();
            });
        }
    });
    _searchingTask->setProgressChanged([this](auto progress) {
        _dispatcher.postToUIThread([=] { emit progressChanged(progress); });
    });
    _searchingTask->start();
}

void LogFile::enterFailed() {
    emit stateChanged();
}

void LogFile::enterComplete() {
    logTableModel()->showIndexedColumns();
    adaptFilter();
    applyFilter();

    if (!_indexingComplete) {
        std::vector<int> widths;
        auto columnCount = logTableModel()->columnCount({});
        widths.push_back(bformat("%d", logTableModel()->rowCount({}) + 1).size());
        for (auto i = 0; i < columnCount - 1; ++i) {
            widths.push_back(_index->maxWidth(i));
        }
        logTableModel()->setColumnWidths(widths);
        _indexingComplete = true;
    }

    emit stateChanged();
}

void LogFile::enterInterrupted() {
    emit stateChanged();
}

void LogFile::searchFromPaused() {
    emit stateChanged();
    searchFromComplete(*_scheduledSearchEvent);
}

void LogFile::reloadFromComplete(sm::ReloadEvent event) {
    if (event.parser) {
        if (_lineParser != event.parser) {
            _columnFilters.clear();
        }
        _lineParser = std::move(event.parser);
    }
    _stream = std::move(event.stream);
    _logTableModel.reset();
    _searchLogTableModel.reset();
    _indexingComplete = false;
    enterParsing();
}

void LogFile::reloadFromParsing(sm::ReloadEvent) {
}

void LogFile::reloadFromIndexing(sm::ReloadEvent) {
}

void LogFile::subscribeToSelectionChanged(LogTableModel* oldModel, LogTableModel* newModel) {
    if (oldModel) {
        disconnect(oldModel);
    }
    connect(newModel, &LogTableModel::selectionChanged, this, [=] {
        auto [first, last] = newModel->getRowSelection();
        auto lineOffset = newModel->lineOffset(first);
        auto row = _logTableModel->findRow(lineOffset);
        _logTableModel->setSelection(row, 0, 0);
    });
}

void LogFile::applyFilter() {
    std::vector<seer::ColumnFilter> filters;
    for (auto i = 0; i < _logTableModel->columnCount({}); ++i) {
        _logTableModel->setFilterActive(i, false);
    }
    for (auto& [c, v] : _columnFilters) {
        _logTableModel->setFilterActive(c + 1, v.size() != _index->getValues(c).size());
        filters.push_back({c, v});
    }
    _index->filter(filters);
    _logTableModel->setIndex(_index.get());
    _logTableModel->setSelection(-1, 0, 0);
}

void LogFile::adaptFilter() {
    for (auto& [c, filters] : _columnFilters) {
        const auto& columnInfos = _index->getValues(c);
        auto values = columnInfos |
            views::transform(&seer::ColumnIndexInfo::value) |
            to<std::vector<std::string>>() |
            actions::sort;
        filters = views::set_intersection(values, filters) | to<std::set<std::string>>;
    }
}

seer::task::Task* LogFile::createIndexingTask(seer::Index* index,
                                              seer::FileParser* fileParser,
                                              seer::ILineParser* lineParser) {
    return new IndexingTask(index, fileParser, lineParser);
}

seer::task::Task* LogFile::createParsingTask(seer::FileParser* fileParser) {
    return new ParsingTask(fileParser);
}

LogTableModel* LogFile::logTableModel() {
    if (!_logTableModel) {
        _logTableModel.reset(new LogTableModel(_fileParser.get()));
    }
    return _logTableModel.get();
}

LogTableModel *LogFile::searchLogTableModel() {
    return _searchLogTableModel.get();
}

const seer::Hist* LogFile::searchHist() {
    return _searchHist.get();
}

seer::ILineParser *LogFile::lineParser() {
    return _lineParser.get();
}

void LogFile::clearFilters() {
    _columnFilters.clear();
    applyFilter();
}

void LogFile::clearFilter(int column) {
    _columnFilters.erase(column - 1);
    applyFilter();
}

void LogFile::excludeValue(int column, const std::string& value) {
    column--;

    auto it = _columnFilters.find(column);
    if (it != end(_columnFilters)) {
        it->second.erase(value);
    } else {
        std::set<std::string> filter;
        for (const auto& info : _index->getValues(column)) {
            if (info.value != value) {
                filter.insert(info.value);
            }
        }
        _columnFilters[column] = filter;
    }

    applyFilter();
}

void LogFile::includeOnlyValue(int column, const std::string& value) {
    _columnFilters[column - 1] = {value};
    applyFilter();
}

void LogFile::requestFilter(int column) {
    if (column == 0 || !_indexingComplete)
        return;
    if (!_lineParser->getColumnFormats().at(column - 1).indexed)
        return;
    auto filterModel = new FilterTableModel(_index->getValues(column - 1));
    emit filterRequested(filterModel, column);
}

void LogFile::setColumnFilter(int column, std::set<std::string> values) {
    if (column == 0)
        return;
    _columnFilters[column - 1] = values;
    applyFilter();
}

std::set<std::string> LogFile::getColumnFilter(int column) {
    return _columnFilters[column - 1];
}

} // namespace gui
