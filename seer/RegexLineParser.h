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
        std::string _magic;
        std::string _name;

    public:
        RegexLineParser(std::string name);
        void load(std::string config);
        bool parseLine(std::string_view line, std::vector<std::string> &columns) override;
        std::vector<ColumnFormat> getColumnFormats() override;
        bool isMatch(std::vector<std::string> sample, std::string_view fileName) override;
        std::string name() const override;
    };

} // namespace seer
