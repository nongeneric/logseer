#include "LogFile.h"

using namespace seer::task;

namespace gui {
    LogFile::LogFile(std::unique_ptr<std::istream>&& stream,
                     std::shared_ptr<seer::ILineParserRepository> repository)
        : _stream(std::move(stream)),
          _repository(std::move(repository)),
          _sm(static_cast<IStateHandler*>(this), _smLogger) {}

    void LogFile::enterParsing() {
        emit stateChanged();
        _lineParser = _repository->resolve(*_stream);
        _fileParser.reset(new seer::FileParser(_stream.get(), _lineParser.get()));
        _parsingTask.reset(new ParsingTask(_fileParser.get()));
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
    }

    void LogFile::enterIndexing() {
        emit stateChanged();
        _index.reset(new seer::Index(_fileParser->lineCount()));
        _indexingTask.reset(
            new IndexingTask(_index.get(), _fileParser.get(), _lineParser.get()));
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
        _searchingTask.reset(new SearchingTask(
            _fileParser.get(), _index.get(), event.text, event.caseSensitive));
        _searchingTask->setStateChanged([=](auto state) {
            assert(state != TaskState::Failed);
            if (state == TaskState::Finished) {
                _searchIndex = _searchingTask->index();
                _dispatcher.postToUIThread([=] {
                    _searchLogTableModel.reset(new LogTableModel(_fileParser.get()));
                    _searchLogTableModel->setIndex(_searchIndex.get());
                    finish();
                });
            }
        });
        _searchingTask->start();
    }

    void LogFile::enterFailed() {
        emit stateChanged();
    }

    void LogFile::enterComplete() {
        _indexingComplete = true;
        emit stateChanged();
    }

    void LogFile::enterInterrupted() {
        emit stateChanged();
    }

    void LogFile::searchFromPaused() {
        emit stateChanged();
        searchFromComplete(*_scheduledSearchEvent);
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

    void LogFile::requestFilter(int column) {
        if (!_lineParser->getColumnFormats()[column].indexed)
            return;
        auto filterModel = new FilterTableModel(_index->getValues(column));
        emit filterRequested(filterModel, column);
    }

    void LogFile::setColumnFilter(int column, std::vector<std::string> values) {
        _logTableModel->setFilterActive(
            column + 1, values.size() != _index->getValues(column).size());
        _columnFilters[column] = values;
        std::vector<seer::ColumnFilter> filters;
        for (auto& [c, v] : _columnFilters) {
            filters.push_back({c, v});
        }
        _index->filter(filters);
        _logTableModel->invalidate();
    }

} // namespace gui
