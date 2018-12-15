#include "FileParser.h"

#include <assert.h>
#include <string>

namespace seer {

    FileParser::FileParser(std::istream* stream, ILineParser* lineParser)
        : _stream(stream), _lineParser(lineParser) {}

    void FileParser::index(std::function<void(uint64_t, uint64_t)> progress) {
        auto lock = std::lock_guard(_mutex);
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

        _stream->seekg(0, std::ios_base::end);
        auto fileSize = _stream->tellg();
        _stream->seekg(0);

        while (std::getline(*_stream, line)) {
            auto pos = _stream->tellg();
            _lineOffsets.add(pos);
            if (pos != -1) {
                progress(pos, fileSize);
            }
            index++;
        }
        _stream->clear();
    }

    uint64_t FileParser::lineCount() {
        auto lock = std::lock_guard(_mutex);
        return _lineOffsets.size() - 1;
    }

    void FileParser::readLine(uint64_t index, std::vector<std::string>& line) {
        std::string text;
        readLine(index, text);
        _lineParser->parseLine(text, line);
    }

    void FileParser::readLine(uint64_t index, std::string &line) {
        auto lock = std::lock_guard(_mutex);
        assert(index < _lineOffsets.size());
        auto offset = _lineOffsets.map(index);
        _stream->seekg(offset);
        std::getline(*_stream, line);
    }

    ILineParser* FileParser::lineParser() const {
        return _lineParser;
    }

} // namespace seer
