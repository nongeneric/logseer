#include "LogFile.h"

namespace gui {

    LogFile::LogFile(std::unique_ptr<std::istream>&& stream,
                     std::shared_ptr<seer::ILineParserRepository> repository)
        : _stream(std::move(stream)), _repository(std::move(repository)) {}

    void LogFile::parse() {
        _lineParser = _repository->resolve(*_stream);
        _fileParser.reset(new seer::FileParser(_stream.get(), _lineParser.get()));
        _index.reset(new seer::Index());
        _index->index(_fileParser.get(), _lineParser.get());
        _logTableModel.reset(new LogTableModel(_index.get(), _fileParser.get()));
        _parsed = true;
    }

    LogTableModel* LogFile::logTableModel() {
        assert(_parsed);
        return _logTableModel.get();
    }

    void LogFile::requestFilter(int column) {
        assert(_parsed);
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
