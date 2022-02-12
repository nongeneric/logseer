#include "LogFile.h"

#include <algorithm>

using namespace seer::task;

namespace gui {

LogFile::LogFile(std::unique_ptr<std::istream> stream,
                 std::shared_ptr<seer::ILineParser> lineParser)
    : _lineParser(lineParser),
      _stream(std::move(stream)),
      _sm(static_cast<IStateHandler*>(this), _smLogger) {}

void LogFile::enterIndexing() {
    emit stateChanged();
    _fileParser = std::make_unique<seer::FileParser>(_stream.get(), _lineParser.get());
    _index = std::make_unique<seer::Index>(_fileParser->lineCount());
    _indexingTask = createIndexingTask(_index.get(), _fileParser.get(), _lineParser.get());
    _indexingTask->setStateChanged([this](auto state) {
        assert(state != TaskState::Failed);
        if (state == TaskState::Finished) {
            _dispatcher.postToUIThread([this] { finish(); });
        } else if (state == TaskState::Stopped) {
            _dispatcher.postToUIThread([this] { finish(); });
        }
    });
    _indexingTask->setProgressChanged([this](auto progress) {
        _dispatcher.postToUIThread([=, this] { emit progressChanged(progress); });
    });
    _indexingTask->start();
}

void LogFile::interruptIndexing() {
    emit stateChanged();
    _indexingTask->stop();
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
    _searchingTask = std::make_unique<SearchingTask>(_fileParser.get(),
                                                     _index.get(),
                                                     event.text,
                                                     event.regex,
                                                     event.caseSensitive,
                                                     event.unicodeAware,
                                                     event.messageOnly);
    _searchingTask->setStateChanged([this](auto state) {
        _dispatcher.postToUIThread([=, this] {
            if (state == TaskState::Finished) {
                auto newSearchTableModel = std::make_unique<LogTableModel>(_fileParser.get());
                subscribeToSelectionChanged(newSearchTableModel.get());
                _searchLogTableModel = std::move(newSearchTableModel);
                _searchHist = _searchingTask->hist();
                _searchIndex = _searchingTask->index();
                _searchLogTableModel->setIndex(_searchIndex.get());
                finish();
            } else if (state == TaskState::Stopped) {
                _searchLogTableModel = nullptr;
                _searchHist.reset();
                _searchIndex.reset();
                finish();
            }
        });
    });
    _searchingTask->setProgressChanged([this](auto progress) {
        _dispatcher.postToUIThread([=, this] { emit progressChanged(progress); });
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
        std::vector<seer::ColumnWidth> widths;
        auto columnCount = logTableModel()->columnCount({});
        widths.push_back({logTableModel()->rowCount({}) - 1, 0});
        for (auto i = 0; i < columnCount - 1; ++i) {
            widths.push_back(_index->maxWidth(i));
        }
        logTableModel()->setColumnWidths(widths);
        _indexingComplete = true;
    }

    emit stateChanged();
}

void LogFile::enterInterrupted() {
    if (_scheduledReload) {
        std::optional<sm::ReloadEvent> event;
        event.swap(_scheduledReload);
        if (event->parser) {
            if (_lineParser != event->parser) {
                _columnFilters.clear();
            }
            _lineParser = std::move(event->parser);
        }
        _stream = std::move(event->stream);
        _logTableModel.reset();
        _searchLogTableModel.reset();
        _indexingComplete = false;
        index();
    } else {
        emit stateChanged();
    }
}

void LogFile::searchFromSearching(sm::SearchEvent /*event*/) {
    if (_searchingTask)
        _searchingTask->stop();
}

void LogFile::subscribeToSelectionChanged(LogTableModel* model) {
    connect(model, &LogTableModel::selectionChanged, this, [=, this] {
        if (auto selection = model->getRowSelection()) {
            auto lineOffset = model->lineOffset(selection->first);
            auto row = _logTableModel->findRow(lineOffset);
            _logTableModel->setSelection(row, 0, 0);
        }
    });
}

void LogFile::applyFilter() {
    int selectedLineOffset = -1;
    if (auto selection = _logTableModel->getRowSelection()) {
        selectedLineOffset = _logTableModel->lineOffset(selection->first);
    }

    std::vector<seer::ColumnFilter> filters;
    for (auto i = 0; i < _logTableModel->columnCount({}); ++i) {
        _logTableModel->setFilterActive(i, false);
    }
    for (auto& [c, v] : _columnFilters) {
        _logTableModel->setFilterActive(c + 1, v.size() != _index->numberOfValues(c));
        filters.push_back({c, v});
    }
    _index->filter(filters);
    _logTableModel->setIndex(_index.get());

    auto selectedRow = _logTableModel->findRow(selectedLineOffset);
    _logTableModel->setSelection(selectedRow, 0, 0);

    for (auto& [_, model] : _filterModels) {
        model->refresh();
    }
}

void LogFile::adaptFilter() {
    for (auto& [c, filters] : _columnFilters) {
        std::vector<std::string> values;
        for (auto& info : _index->getValues(c)) {
            values.push_back(info.value);
        }

        std::set<std::string> intersection;
        std::set_intersection(begin(values),
                              end(values),
                              begin(filters),
                              end(filters),
                              std::inserter(intersection, begin(intersection)));
        filters = intersection;
    }
}

std::shared_ptr<seer::task::Task> LogFile::createIndexingTask(seer::Index* index,
                                                              seer::FileParser* fileParser,
                                                              seer::ILineParser* lineParser) {
    return std::make_shared<IndexingTask>(index, fileParser, lineParser);
}

LogTableModel* LogFile::logTableModel() {
    if (!_logTableModel) {
        _logTableModel = std::make_unique<LogTableModel>(_fileParser.get());
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

void LogFile::reload(std::shared_ptr<std::istream> stream,
                     std::shared_ptr<seer::ILineParser> parser) {
    _scheduledReload = {stream, parser};
    _filterModels.clear();
    interrupt();
}

std::string LogFile::dbgStateName() const {
    std::string name;
    _sm.visit_current_states([&](auto state) {
        name = state.c_str();
    });
    return name;
}

void LogFile::requestFilter(int column) {
    if (column == 0 || !_indexingComplete)
        return;
    auto format = _lineParser->getColumnFormats().at(column - 1);
    if (!format.indexed)
        return;

    auto it = _filterModels.find(column - 1);
    if (it == end(_filterModels)) {
        auto model = std::make_shared<FilterTableModel>([=, this] { return _index->getValues(column - 1); });
        std::tie(it, std::ignore) = _filterModels.emplace(column - 1, model);
    }

    auto& filterModel = it->second;
    emit filterRequested(filterModel, column, format.header);
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
