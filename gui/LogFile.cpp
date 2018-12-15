#include "LogFile.h"

namespace gui {

    LogFile::LogFile(std::unique_ptr<std::istream>&& stream,
                     std::shared_ptr<seer::ILineParserRepository> repository)
        : _stream(std::move(stream)), _repository(std::move(repository)) {}

    void LogFile::parse() {
        _state = LogFileState::Parsing;
        _lineParser = _repository->resolve(*_stream);
        _fileParser.reset(new seer::FileParser(_stream.get(), _lineParser.get()));
        _index.reset(new seer::Index());
        _parsingTask.reset(new seer::ParsingTask(_fileParser.get()));
        _parsingTask->setStateChanged([=] (auto state) {
            assert(state != seer::TaskState::Failed);
            if (state == seer::TaskState::Finished) {
                _state = LogFileState::Parsed;
                emit parsingComplete();
            }
        });
        _parsingTask->setProgressChanged([=] (auto progress) {
            emit parsingProgress(progress);
        });
        _parsingTask->start();
    }

    void LogFile::index() {
        assert(_state == LogFileState::Parsed);
        _state = LogFileState::Indexing;

        _indexingTask.reset(new seer::IndexingTask(_index.get(), _fileParser.get(), _lineParser.get()));
        _indexingTask->setStateChanged([=] (auto state) {
            assert(state != seer::TaskState::Failed);
            if (state == seer::TaskState::Finished) {
                _state = LogFileState::Complete;
                emit indexingComplete();
            }
        });
        _indexingTask->setProgressChanged([=] (auto progress) {
            emit indexingProgress(progress);
        });
        _indexingTask->start();
    }

    LogTableModel* LogFile::logTableModel() {
        assert(_state == LogFileState::Complete ||
               _state == LogFileState::Indexing || _state == LogFileState::Parsed);
        if (!_logTableModel) {
            _logTableModel.reset(new LogTableModel(_fileParser.get()));
        }
        return _logTableModel.get();
    }

    LogTableModel *LogFile::searchLogTableModel(std::string text, bool caseSensitive) {
        _searchIndex.reset(new seer::Index(*_index));
        _searchIndex->search(_fileParser.get(), text, caseSensitive);
        _searchLogTableModel.reset(new LogTableModel(_fileParser.get()));
        return _searchLogTableModel.get();
    }

    LogFileState LogFile::state() const {
        return _state;
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
