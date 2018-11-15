#include "FileParser.h"

#include <assert.h>
#include <string>

namespace seer {

    FileParser::FileParser(std::istream* stream, ILineParser* lineParser)
        : _stream(stream), _lineParser(lineParser) {}

    void FileParser::index(LineHandler handler) {
        _indexed = true;
        std::string line;
        std::vector<std::string> columns;
        uint64_t index = 0;
        _lineOffsets.reset(32, [=](uint64_t offset) {
            _stream->seekg(offset);
            std::string line;
            std::getline(*_stream, line);
            return _stream->tellg();
        });
        _lineOffsets.add(0);
        while (std::getline(*_stream, line)) {
            _lineOffsets.add(_stream->tellg());
            _lineParser->parseLine(line, columns);
            handler(index, columns);
            index++;
        }
        _stream->clear();
    }

    uint64_t FileParser::lineCount() {
        return _lineOffsets.size() - 1;
    }

    void FileParser::readLine(uint64_t index, std::vector<std::string>& line) {
        assert(index < _lineOffsets.size());
        auto offset = _lineOffsets.map(index);
        _stream->seekg(offset);
        std::string text;
        std::getline(*_stream, text);
        _lineParser->parseLine(text, line);
    }

    ILineParser* FileParser::lineParser() const {
        return _lineParser;
    }

} // namespace seer
