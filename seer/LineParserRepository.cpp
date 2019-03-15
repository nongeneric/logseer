#include "LineParserRepository.h"
#include "RegexLineParser.h"

#include "Log.h"
#include <boost/filesystem.hpp>
#include <regex>
#include <limits>

using namespace boost::filesystem;

namespace seer {

    class DefaultLineParser : public ILineParser {
    public:
        bool parseLine(std::string_view line, std::vector<std::string> &columns) override {
            columns.clear();
            columns.push_back(line.data());
            return true;
        }

        std::vector<ColumnFormat> getColumnFormats() override {
            return {{"Message", false}};
        }

        bool isMatch(std::vector<std::string>, std::string_view) override {
            return true;
        }

        std::string name() const override {
            return "default";
        }
    };

    LineParserRepository::LineParserRepository() {
        _parsers[std::numeric_limits<int>::max()] = std::make_shared<DefaultLineParser>();
    }

    void LineParserRepository::addRegexParser(std::string name, int priority, std::string json) {
        auto parser = std::make_shared<RegexLineParser>(name);
        try {
            parser->load(json);
            _parsers[priority] = parser;
        } catch (std::exception& e) {
            log_infof("parser config [%s] can't be loaded: [%s]", name, e.what());
        }
    }

    std::shared_ptr<ILineParser> LineParserRepository::resolve(std::istream& stream) {
        std::vector<std::string> sample;
        std::string line;
        while (sample.size() < 10 && std::getline(stream, line)) {
            sample.push_back(line);
        }
        for (auto& [_, parser] : _parsers) {
            log_infof("trying parser [%s]", parser->name());
            if (parser->isMatch(sample, "")) {
                stream.clear();
                stream.seekg(0);
                log_infof("selected parser [%s]", parser->name());
                return parser;
            }
        }
        return nullptr;
    }

} // namespace seer
