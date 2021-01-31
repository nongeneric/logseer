#include "RegexLineParser.h"

#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>
#include <seer/bformat.h>
#include <sstream>
#include <algorithm>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

using namespace nlohmann;

namespace seer {

RegexLineParser::RegexLineParser(std::string name) : _name(name) {}

std::string reErrorToString(int code) {
    std::string error(1<<10, 0);
    pcre2_get_error_message(code, (PCRE2_UCHAR*)&error[0], error.size());
    return error;
}

void RegexLineParser::load(std::string config) {
    std::string rePattern;

    try {
        std::stringstream ss{config};
        json j;
        ss >> j;
        auto description = j["description"].get<std::string>();
        rePattern = j["regex"].get<std::string>();

        auto& columns = j["columns"];
        auto magic = j["magic"];
        if (!magic.is_null()) {
            _magic = magic.get<std::string>();
        }

        for (auto it = begin(columns); it != end(columns); ++it) {
            auto name = (*it)["name"].get<std::string>();
            auto group = (*it)["group"].get<int>();
            auto indexed = it->value("indexed", false);
            auto autosize = it->value("autosize", false);
            _formats.push_back({name, group, indexed, autosize});
        }

        auto colors = j["colors"];
        if (!colors.is_null()) {
            for (auto it = begin(colors); it != end(colors); ++it) {
                auto columnName = (*it)["column"].get<std::string>();
                auto value = (*it)["value"].get<std::string>();
                auto color = std::stoi((*it)["color"].get<std::string>(), 0, 16);
                auto column = std::find_if(begin(_formats), end(_formats), [&] (auto& format) {
                    return format.name == columnName;
                });
                if (column == end(_formats))
                    continue;
                auto columnIndex = static_cast<int>(std::distance(begin(_formats), column));
                _colors.push_back({columnIndex, value, static_cast<uint32_t>(color)});
            }
        }
    } catch (json::exception& e) {
        throw JsonParserException(e.what());
    }

    int reError = 0;
    PCRE2_SIZE errorOffset;
    _re.reset(pcre2_compile((PCRE2_SPTR8)rePattern.c_str(),
                            PCRE2_ZERO_TERMINATED,
                            0,
                            &reError,
                            &errorOffset,
                            nullptr),
              pcre2_code_free_8);

    if (!_re)
        throw RegexpSyntaxException(reErrorToString(reError));

    reError = pcre2_jit_compile(_re.get(), PCRE2_JIT_COMPLETE);

    if (reError)
        throw RegexpSyntaxException(reErrorToString(reError));

    auto matchData = std::shared_ptr<pcre2_match_data>(
        pcre2_match_data_create_from_pattern(_re.get(), nullptr),
        pcre2_match_data_free);

    int groupCount = pcre2_get_ovector_count(matchData.get());
    for (auto& format : _formats) {
        if (format.group < 0 || format.group >= groupCount)
            throw RegexpOutOfBoundGroupReferenceException(bformat(
                "Column \"%s\" references a nonexistent group \"%d\" (there are only %d groups in the regex).",
                format.name,
                format.group,
                groupCount));
    }
}

bool RegexLineParser::parseLine(std::string_view line,
                                std::vector<std::string>& columns) {
    auto matchData = std::shared_ptr<pcre2_match_data>(
        pcre2_match_data_create_from_pattern(_re.get(), nullptr),
        pcre2_match_data_free);

    auto rc = pcre2_jit_match(_re.get(),
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
        assert(static_cast<size_t>(i) <
               pcre2_get_ovector_count(matchData.get()));
        auto group = line.data() + vec[2 * i];
        auto len = vec[2 * i + 1] - vec[2 * i];
        columns.emplace_back(group, len);
    }
    return true;
}

std::vector<ColumnFormat> RegexLineParser::getColumnFormats() {
    std::vector<ColumnFormat> formats;
    for (auto& format : _formats) {
        formats.push_back({format.name, format.indexed, format.autosize});
    }
    return formats;
}

uint32_t RegexLineParser::rgb(const std::vector<std::string>& columns) const {
    for (auto& color : _colors) {
        if (color.column < static_cast<int>(columns.size()) &&
            color.value == columns[color.column])
            return color.color;
    }
    return 0;
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
