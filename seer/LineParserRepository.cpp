#include "LineParserRepository.h"

#include <regex>

namespace seer {

    class Ps3EmuLineParser : public seer::ILineParser {
        std::regex _re;

    public:
        Ps3EmuLineParser() {
            _re = R"(^((\d+:\d+\.\d+) )?(\w+)( \[(.*?)\])?( ([0-9a-f]*?))? (\w+): (.*))";
        }

        bool parseLine([[maybe_unused]] std::string_view line,
                       [[maybe_unused]] std::vector<std::string>& columns) override {

            std::smatch match;
            auto s = std::string(line);
            if (std::regex_search(s, match, _re) && match.size() > 1) {
                columns = {
                    match.str(2),
                    match.str(3),
                    match.str(5),
                    match.str(7),
                    match.str(8),
                    match.str(9),
                };
            }

            return true;
        }

        std::vector<seer::ColumnFormat> getColumnFormats() override {
            return {{"Timestamp", false},
                    {"Component", true},
                    {"Thread", true},
                    {"Offset", false},
                    {"Level", true},
                    {"Message", false}};
        }

        bool isMatch([[maybe_unused]] std::string_view sample,
                     [[maybe_unused]] std::string_view fileName) override {
            return true;
        }
    };

    std::shared_ptr<ILineParser> LineParserRepository::resolve(std::istream&) {
        return std::make_shared<Ps3EmuLineParser>();
    }

} // namespace seer
