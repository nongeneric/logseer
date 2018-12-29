#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace seer {

    struct ColumnFormat {
        std::string header;
        bool indexed;
    };

    struct ILineParser {
        virtual bool parseLine(std::string_view line,
                               std::vector<std::string>& columns) = 0;
        virtual std::vector<ColumnFormat> getColumnFormats() = 0;
        virtual bool isMatch(std::vector<std::string> sample, std::string_view fileName) = 0;
        virtual ~ILineParser() = default;
    };
} // namespace seer
