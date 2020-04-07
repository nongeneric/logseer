#pragma once

#include "ILineParser.h"
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
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
    bool autosize;
};

class JsonParserException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class RegexpSyntaxException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class RegexpOutOfBoundGroupReferenceException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};


class RegexLineParser : public ILineParser {
    std::vector<RegexColumnFormat> _formats;
    std::vector<RegexColumnColor> _colors;
    std::string _magic;
    std::string _name;
    std::shared_ptr<pcre2_real_code_8> _re;

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
