#include "RegexLineParser.h"

#include <json.hpp>
#include <sstream>

using namespace nlohmann;

namespace seer {

    RegexLineParser::RegexLineParser() {}

    void RegexLineParser::load(std::string config) {
        std::stringstream ss{config};
        json j;
        ss >> j;
        auto description = j["description"].get<std::string>();
        _re = j["regex"].get<std::string>();
        auto& columns = j["columns"];
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

    bool RegexLineParser::isMatch([[maybe_unused]] std::string_view sample,
                                  [[maybe_unused]] std::string_view fileName) {
        return true;
    }

} // namespace seer
