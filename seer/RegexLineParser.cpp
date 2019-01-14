#include "RegexLineParser.h"

#include <json.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>

using namespace nlohmann;

namespace seer {

    RegexLineParser::RegexLineParser(std::string name) : _name(name) {}

    void RegexLineParser::load(std::string config) {
        std::stringstream ss{config};
        json j;
        ss >> j;
        auto description = j["description"].get<std::string>();
        _re = j["regex"].get<std::string>();
        auto& columns = j["columns"];
        auto magic = j["magic"];
        if (!magic.is_null()) {
            _magic = magic.get<std::string>();
        }
        for (auto it = begin(columns); it != end(columns); ++it) {
            auto name = (*it)["name"].get<std::string>();
            auto group = (*it)["group"].get<int>();
            auto indexed = (*it)["indexed"].get<bool>();
            _formats.push_back({name, group, indexed});
        }
    }

    bool RegexLineParser::parseLine(std::string_view line,
                                    std::vector<std::string>& columns) {
        std::smatch match;
        auto s = std::string(line);
        if (std::regex_search(s, match, _re) && match.size() > 1) {
            columns.clear();
            for (auto& format : _formats) {
                columns.push_back(match.str(format.group));
            }
            return true;
        }
        return false;
    }

    std::vector<ColumnFormat> RegexLineParser::getColumnFormats() {
        std::vector<ColumnFormat> formats;
        for (auto& format : _formats) {
            formats.push_back({format.name, format.indexed});
        }
        return formats;
    }

    bool RegexLineParser::isMatch([[maybe_unused]] std::vector<std::string> sample,
                                  [[maybe_unused]] std::string_view fileName) {
        if (sample.empty())
            return false;
        if (!_magic.empty()) {
            return boost::starts_with(sample.front(), _magic);
        }
        std::vector<std::string> columns;
        return parseLine(sample[0], columns);
    }

    std::string RegexLineParser::name() const{
        return _name;
    }

} // namespace seer
