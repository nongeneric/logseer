#pragma once

#include "ILineParser.h"
#include <string>
#include <vector>
#include <stdint.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace seer {

    struct RegexColumnColor {
        int column;
        std::string value;
        uint32_t color;
    };

    struct RegexColumnFormat {
        std::string name;
        int group;
        bool indexed;
    };

    class RegexLineParser : public ILineParser {
        std::vector<RegexColumnFormat> _formats;
        std::vector<RegexColumnColor> _colors;
        std::string _magic;
        std::string _name;
        pcre2_code* _re;

    public:
        RegexLineParser(std::string name);
        void load(std::string config);
        bool parseLine(std::string_view line, std::vector<std::string> &columns) override;
        std::vector<ColumnFormat> getColumnFormats() override;
        bool isMatch(std::vector<std::string> sample, std::string_view fileName) override;
        uint32_t rgb(const std::vector<std::string> &columns) const override;
        std::string name() const override;
    };

} // namespace seer
