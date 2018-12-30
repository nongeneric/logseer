#include "LineParserRepository.h"
#include "RegexLineParser.h"

#include "Log.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <limits>

using namespace std::filesystem;

namespace {
    std::vector<uint8_t> read_all_bytes(std::string_view path) {
        auto f = fopen(begin(path), "r");
        if (!f)
            throw std::runtime_error("can't open file");
        fseek(f, 0, SEEK_END);
        auto filesize = ftell(f);
        std::vector<uint8_t> res(filesize);
        fseek(f, 0, SEEK_SET);
        fread(&res[0], 1, res.size(), f);
        fclose(f);
        return res;
    }

    std::string read_all_text(std::string_view path) {
        auto vec = read_all_bytes(path);
        return std::string((const char*)&vec[0], vec.size());
    }
}

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
    };

    void LineParserRepository::init() {
        _parsers[std::numeric_limits<int>::max()] = std::make_shared<DefaultLineParser>();

        auto dir = path(std::getenv("HOME")) / ".logseer" / "regex";
        if (!exists(dir))
            return;

        std::regex rxFileName{R"(^(\d\d\d)_(.*?)\.json$)"};

        for (auto& p : directory_iterator(dir)) {
            if (!p.is_regular_file())
                continue;
            std::smatch match;
            auto fileName = p.path().filename().string();
            if (!std::regex_search(fileName, match, rxFileName)) {
                log_infof("parser config name '%s' doesn't have the right format", fileName);
                continue;
            }
            auto priority = std::stoi(match.str(1));
            //auto name = match.str(2);
            auto parser = std::make_shared<RegexLineParser>();
            try {
                parser->load(read_all_text(p.path().string()));
                _parsers[priority] = parser;
            } catch (std::exception& e) {
                log_infof("parser config '%s' can't be loaded: %s", fileName, e.what());
            }
        }
    }

    std::shared_ptr<ILineParser> LineParserRepository::resolve(std::istream& stream) {
        std::vector<std::string> sample;
        std::string line;
        while (sample.size() < 10 && std::getline(stream, line)) {
            sample.push_back(line);
        }
        for (auto& [_, parser] : _parsers) {
            if (parser->isMatch(sample, ""))
                return parser;
        }
        return nullptr;
    }

} // namespace seer
