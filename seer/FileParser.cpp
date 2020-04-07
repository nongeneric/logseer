#include "FileParser.h"

#include <assert.h>
#include <string>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/locale/encoding_utf.hpp>

namespace seer {

void FileParser::initConverter() {
    _stream->seekg(0);
    std::string line;
    std::getline(*_stream, line);

    std::string bom8{"\xEF\xBB\xBF"};
    std::string bom16be{"\xFE\xFF"};
    std::string bom16le{"\xFF\xFE"};
    std::string bom32be{"\0\0\xFE\xFF", 4};
    std::string bom32le{"\xFE\xFF\0\0", 4};

    if (boost::starts_with(line, bom8)) {
        _bomSize = bom8.size();
    } else if (boost::starts_with(line, bom32be)) {
        _bomSize = bom32be.size();
        _eolLeftPadding = 3;
        _convert = [=] (auto& text) {
            auto b = reinterpret_cast<char32_t*>(&text[0]);
            auto e = b + text.size() / sizeof(char32_t);
            for (auto it = b; it != e; ++it) {
                *it = (*it >> 24)
                    | ((*it >> 8) & 0xff00)
                    | ((*it << 8) & 0xff0000)
                    | ((*it & 0xff) << 24);
            }
            text = boost::locale::conv::utf_to_utf<char>(b, e);
        };
    } else if (boost::starts_with(line, bom32le)) {
        _bomSize = bom32le.size();
        _handleZeros = false;
        _eolRightPadding = 3;
        _convert = [=] (auto& text) {
            auto b = reinterpret_cast<char32_t*>(&text[0]);
            auto e = b + text.size() / sizeof(char32_t);
            text = boost::locale::conv::utf_to_utf<char>(b, e);
        };
    } else if (boost::starts_with(line, bom16be)) {
        _bomSize = bom16be.size();
        _eolLeftPadding = 1;
        _convert = [=] (auto& text) {
            auto b = reinterpret_cast<char16_t*>(&text[0]);
            auto e = b + text.size() / sizeof(char16_t);
            for (auto it = b; it != e; ++it) {
                *it = (*it >> 8) | ((*it & 0xff) << 8);
            }
            text = boost::locale::conv::utf_to_utf<char>(b, e);
        };
    } else if (boost::starts_with(line, bom16le)) {
        _bomSize = bom16le.size();
        _handleZeros = false;
        _eolRightPadding = 1;
        _convert = [=] (auto& text) {
            auto b = reinterpret_cast<char16_t*>(&text[0]);
            auto e = b + text.size() / sizeof(char16_t);
            text = boost::locale::conv::utf_to_utf<char>(b, e);
        };
    }
}

FileParser::FileParser(std::istream* stream, ILineParser* lineParser)
    : _stream(stream), _lineParser(lineParser) {
    initConverter();
}

void FileParser::index(std::function<void(uint64_t, uint64_t)> progress,
                       std::function<bool()> stopRequested) {
    auto lock = std::lock_guard(_mutex);
    _indexed = true;
    std::string line;
    std::vector<std::string> columns;
    uint64_t index = 0;
    _lineOffsets.reset(32, [this](uint64_t offset) {
        _stream->seekg(offset);
        std::string line;
        std::getline(*_stream, line);
        _stream->ignore(_eolRightPadding);
        return _stream->tellg();
    });
    _lineOffsets.add(_bomSize);

    _stream->seekg(0, std::ios_base::end);
    auto fileSize = _stream->tellg();
    _stream->seekg(0);

    _stream->ignore(_bomSize);
    while (std::getline(*_stream, line)) {
        _stream->ignore(_eolRightPadding);
        auto pos = _stream->tellg();
        _lineOffsets.add(pos);
        if (pos != -1 && progress) {
            progress(pos, fileSize);
        }
        if (stopRequested())
            return;
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

void FileParser::readLine(uint64_t index, std::string& line) {
    auto lock = std::lock_guard(_mutex);
    assert(index < _lineOffsets.size());
    if (index != _currentIndex) {
        auto offset = _lineOffsets.map(index);
        _stream->seekg(offset);
        _currentIndex = index;
    }
    auto lastLine = index == _lineOffsets.size() - 2;
    if (_lastLineSize != -1 && lastLine) {
        line.resize(_lastLineSize);
        _stream->read(&line[0], _lastLineSize);
        if (_convert)
            _convert(line);
    } else {
        std::getline(*_stream, line);
        if (!lastLine) {
            _stream->ignore(_eolRightPadding);
            if ((int)line.size() >= _eolLeftPadding) { // TODO: ssize
                line.resize(line.size() - _eolLeftPadding);
            }
        }

        if (lastLine) {
            if (_handleZeros) {
                std::array<char, 1> delim {'\0'};
                boost::trim_right_if(line, boost::is_any_of(delim));
            }
            if (_lastLineSize == -1) {
                _lastLineSize = line.size();
            }
        }

        if (_convert)
            _convert(line);
    }
    assert(!_stream->fail());
    _currentIndex++;
}

ILineParser* FileParser::lineParser() const {
    return _lineParser;
}

} // namespace seer
