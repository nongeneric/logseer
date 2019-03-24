#pragma once

#include "ILineParser.h"
#include <string>
#include <vector>
#include <memory>
#include <stdint.h>

struct pcre2_real_code_8;

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
        pcre2_real_code_8* _re;

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
