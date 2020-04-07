#pragma once

#include "ILineParser.h"
#include "OffsetIndex.h"
#include <functional>
#include <istream>
#include <string_view>
#include <vector>
#include <mutex>

namespace seer {

class FileParser {
    OffsetIndex _lineOffsets;
    std::istream* _stream;
    ILineParser* _lineParser;
    bool _indexed = false;
    size_t _currentIndex = -1;
    std::mutex _mutex;
    int64_t _lastLineSize = -1;

public:
    FileParser(std::istream* stream, ILineParser* lineParser);
    void index(std::function<void(uint64_t, uint64_t)> progress = {},
               std::function<bool()> stopRequested = []{ return false; });
    uint64_t lineCount();
    void readLine(uint64_t index, std::vector<std::string>& line);
    void readLine(uint64_t index, std::string& line);
    ILineParser* lineParser() const;
};

} // namespace seer
