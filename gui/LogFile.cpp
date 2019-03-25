#include "LogFile.h"

using namespace seer::task;

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
                _dispatcher.postToUIThread([=] { index(); });
            }
        });
        _parsingTask->setProgressChanged([=](auto progress) {
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
        _indexingTask->setStateChanged([=](auto state) {
            assert(state != TaskState::Failed);
            if (state == TaskState::Finished) {
                _dispatcher.postToUIThread([=] { finish(); });
            } else if (state == TaskState::Paused) {
                _dispatcher.postToUIThread([=] { paused(); });
            } else if (state == TaskState::Stopped) {
                _dispatcher.postToUIThread([=] { finish(); });
            }
        });
        _indexingTask->setProgressChanged([=](auto progress) {
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
        _searchingTask->setStateChanged([=](auto state) {
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
        _searchingTask->setProgressChanged([=](auto progress) {
            _dispatcher.postToUIThread([=] { emit progressChanged(progress); });
        });
        _searchingTask->start();
    }

    void LogFile::enterFailed() {
        emit stateChanged();
    }

    void LogFile::enterComplete() {
        _indexingComplete = true;
        logTableModel()->showIndexedColumns();
        std::vector<int> widths;
        auto columnCount = logTableModel()->columnCount({});
        widths.push_back(seer::ssnprintf("%d", logTableModel()->rowCount({}) + 1).size());
        for (auto i = 0; i < columnCount - 1; ++i) {
            widths.push_back(_index->maxWidth(i));
        }
        logTableModel()->setColumnWidths(widths);
        auto searchModel = searchLogTableModel();
        if (searchModel) {
            searchModel->setColumnWidths(widths);
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

    void LogFile::subscribeToSelectionChanged(LogTableModel *oldModel, LogTableModel *newModel) {
        if (oldModel) {
            disconnect(oldModel);
        }
        connect(newModel, &LogTableModel::selectionChanged, this, [=] {
            auto [first, last] = newModel->getSelection();
            auto lineOffset = newModel->lineOffset(first);
            auto row = _logTableModel->findRow(lineOffset);
            _logTableModel->setSelection(row);
        });
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
        _logTableModel->setFilterActive(
            column, values.size() != _index->getValues(column - 1).size());
        _columnFilters[column - 1] = values;
        std::vector<seer::ColumnFilter> filters;
        for (auto& [c, v] : _columnFilters) {
            filters.push_back({c, v});
        }
        _index->filter(filters);
        _logTableModel->setIndex(_index.get());
        _logTableModel->setSelection(-1);
    }

} // namespace gui
