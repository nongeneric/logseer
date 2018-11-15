#include "LogFile.h"

#include <fstream>
#include <regex>

namespace gui {

    class Ps3EmuLineParser : public seer::ILineParser {
        std::regex _re;

    public:
        Ps3EmuLineParser() {
            _re =
                R"(^((\d+:\d+\.\d+) )?(\w+)( \[(.*?)\])?( ([0-9a-f]*?))? (\w+): (.*))";
        }

        bool parseLine([[maybe_unused]] std::string_view line,
                       [[maybe_unused]] std::vector<std::string>& columns) override {

            std::smatch match;
            auto s = std::string(line);
            if (std::regex_search(s, match, _re) && match.size() > 1) {
                columns = {
                    match.str(2),
                    match.str(3),
                    match.str(5),
                    match.str(7),
                    match.str(8),
                    match.str(9),
                };
            }

            return true;
        }

        std::vector<seer::ColumnFormat> getColumnFormats() override {
            return {{"Timestamp", false},
                    {"Component", true},
                    {"Thread", true},
                    {"Offset", false},
                    {"Level", true},
                    {"Message", false}};
        }

        bool isMatch([[maybe_unused]] std::string_view sample,
                     [[maybe_unused]] std::string_view fileName) override {
            return true;
        }
    };

    LogFile::LogFile(std::string path) : _path(path) {}

    void LogFile::parse() {
        _lineParser.reset(new Ps3EmuLineParser());
        auto f = new std::ifstream(_path);
        _fileParser.reset(new seer::FileParser(f, _lineParser.get()));
        _index.reset(new seer::Index());
        _index->index(_fileParser.get(), _lineParser.get());
        _logTableModel.reset(new LogTableModel(_index.get(), _fileParser.get()));
    }

    LogTableModel* LogFile::logTableModel() {
        return _logTableModel.get();
    }

    void LogFile::requestFilter(int column) {
        if (column == 0 || !_lineParser->getColumnFormats()[column].indexed)
            return;
        auto filterModel = new FilterTableModel(_index->getValues(column));
        emit filterRequested(filterModel, column);
    }

    void LogFile::setColumnFilter(int column, std::vector<std::string> values) {
        _columnFilters[column] = values;
        std::vector<seer::ColumnFilter> filters;
        for (auto& [c, v] : _columnFilters) {
            filters.push_back({c, v});
        }
        _index->filter(filters);
        _logTableModel->invalidate();
    }

} // namespace gui
