#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <stdint.h>

namespace seer {

struct ColumnFormat {
    std::string header;
    bool indexed;
    bool autosize;
};

class ILineParserContext {
public:
    virtual ~ILineParserContext() = default;
};

class ILineParser {
public:
    virtual bool parseLine(std::string_view line,
                           std::vector<std::string>& columns,
                           ILineParserContext& context) = 0;
    virtual std::vector<ColumnFormat> getColumnFormats() = 0;
    virtual bool isMatch(const std::vector<std::string>& sample, std::string_view fileName) = 0;
    virtual std::string name() const = 0;
    virtual uint32_t rgb(std::vector<std::string> const&) const { return 0; }
    virtual std::unique_ptr<ILineParserContext> createContext() const = 0;
    virtual ~ILineParser() = default;
};

} // namespace seer
