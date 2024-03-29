#include "LineParserRepository.h"
#include "RegexLineParser.h"
#include "FileParser.h"

#include "Log.h"
#include <filesystem>
#include <regex>
#include <limits>

using namespace std::filesystem;

namespace seer {

constexpr int g_sampleBytes = 1000;

class DefaultLineParser : public ILineParser {
public:
    bool parseLine(std::string_view line, std::vector<std::string> &columns, ILineParserContext& /*context*/) override {
        columns.clear();
        columns.push_back(line.data());
        return true;
    }

    std::vector<ColumnFormat> getColumnFormats() override {
        return {{"Message", false, false}};
    }

    bool isMatch(const std::vector<std::string>&, std::string_view) override {
        return true;
    }

    std::string name() const override {
        return "default";
    }

    std::unique_ptr<ILineParserContext> createContext() const override { return {}; }
};

LineParserRepository::LineParserRepository(bool initializeDefaultParser) {
    if (initializeDefaultParser) {
        _parsers[std::numeric_limits<int>::max()] = std::make_shared<DefaultLineParser>();
    }
}

std::optional<std::string> LineParserRepository::addRegexParser(std::string name,
                                                                int priority,
                                                                std::string json) {
    auto parser = std::make_shared<RegexLineParser>(name);
    try {
        parser->load(json);
        _parsers[priority] = parser;
        return {};
    } catch (std::exception& e) {
        log_info(e.what());
        return e.what();
    }
}

std::shared_ptr<ILineParser> LineParserRepository::resolve(std::istream& stream) {
    std::string line(g_sampleBytes, '\0');
    stream.read(&line[0], line.size());
    std::stringstream ss(line);

    FileParser fp(&ss, nullptr);
    fp.index();
    std::vector<std::string> sample;
    for (auto i = 0u; i < fp.lineCount(); ++i) {
        fp.readLine(i, line);
        sample.push_back(line);
    }

    for (auto& [_, parser] : _parsers) {
        log_infof("trying parser [{}]", parser->name());
        if (parser->isMatch(sample, "") || parser == (--end(_parsers))->second) {
            stream.clear();
            stream.seekg(0);
            log_infof("selected parser [{}]", parser->name());
            return parser;
        }
    }
    return nullptr;
}

const ParserMap& LineParserRepository::parsers() const {
    return _parsers;
}

} // namespace seer
