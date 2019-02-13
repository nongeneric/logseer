#include "RegexLineParser.h"

#include <nlohmann/json.hpp>
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
        auto rePattern = j["regex"].get<std::string>();

        int reError;
        PCRE2_SIZE errorOffset;
        _re = pcre2_compile((PCRE2_SPTR8)rePattern.c_str(),
                            PCRE2_ZERO_TERMINATED,
                            0,
                            &reError,
                            &errorOffset,
                            nullptr);
        assert(_re);
        reError = pcre2_jit_compile(_re, PCRE2_JIT_COMPLETE);
        assert(reError == 0);

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
        std::shared_ptr<pcre2_match_data> matchData(
            pcre2_match_data_create_from_pattern(_re, nullptr),
            pcre2_match_data_free);
        auto rc = pcre2_match(_re,
                              (PCRE2_SPTR8)line.data(),
                              line.size(),
                              0,
                              0,
                              matchData.get(),
                              nullptr);
        if (rc < 0)
            return false;

        auto vec = pcre2_get_ovector_pointer(matchData.get());
        columns.clear();

        for (auto& format : _formats) {
            auto i = format.group;
            assert(i < rc);
            auto group = line.data() + vec[2 * i];
            auto len = vec[2 * i + 1] - vec[2 * i];
            columns.emplace_back(group, len);
        }
        return true;
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
