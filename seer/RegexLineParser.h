#pragma once

#include "ILineParser.h"
#include <string>
#include <vector>
#include <regex>

namespace seer {
    struct RegexColumnFormat {
        std::string name;
        int group;
        bool indexed;
    };

    class RegexLineParser : public ILineParser {
        std::vector<RegexColumnFormat> _formats;
        std::regex _re;

    public:
        RegexLineParser();
        void load(std::string config);
        bool parseLine(std::string_view line, std::vector<std::string> &columns) override;
        std::vector<ColumnFormat> getColumnFormats() override;
        bool isMatch(std::string_view sample, std::string_view fileName) override;
    };

} // namespace seer
