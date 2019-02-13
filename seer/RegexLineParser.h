#pragma once

#include "ILineParser.h"
#include <string>
#include <vector>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace seer {
    struct RegexColumnFormat {
        std::string name;
        int group;
        bool indexed;
    };

    class RegexLineParser : public ILineParser {
        std::vector<RegexColumnFormat> _formats;
        std::string _magic;
        std::string _name;
        pcre2_code* _re;

    public:
        RegexLineParser(std::string name);
        void load(std::string config);
        bool parseLine(std::string_view line, std::vector<std::string> &columns) override;
        std::vector<ColumnFormat> getColumnFormats() override;
        bool isMatch(std::vector<std::string> sample, std::string_view fileName) override;
        std::string name() const override;
    };

} // namespace seer
