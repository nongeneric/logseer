#pragma once

#include "ILineParser.h"
#include "OffsetIndex.h"
#include <functional>
#include <istream>
#include <string_view>
#include <vector>

namespace seer {

    using LineHandler =
        std::function<void(uint64_t, std::vector<std::string> const&)>;

    class FileParser {
        OffsetIndex _lineOffsets;
        std::istream* _stream;
        ILineParser* _lineParser;
        bool _indexed = false;

    public:
        FileParser(std::istream* stream, ILineParser* lineParser);
        void index(LineHandler handler);
        uint64_t lineCount();
        void readLine(uint64_t index, std::vector<std::string>& line);
        ILineParser* lineParser() const;
    };

} // namespace seer
