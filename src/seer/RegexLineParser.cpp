#include "RegexLineParser.h"

#include <seer/lua/LuaInterpreter.h>

#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <sstream>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

using namespace nlohmann;

namespace seer {

class RegexLineParserContext : public ILineParserContext {
public:
    std::shared_ptr<pcre2_match_data> matchData;
};

class MagicLogDetector : public ILogDetector {
    std::string _magic;
public:
    MagicLogDetector(std::string magic) : _magic(std::move(magic)) {}

    bool isMatch(const std::vector<std::string>& lines, [[maybe_unused]] std::string_view fileName) override {
        if (lines.empty())
            return false;
        return boost::starts_with(lines.front(), _magic);
    }
};

class DefaultLogDetector : public ILogDetector {
    RegexLineParser* _parser;

public:
    DefaultLogDetector(RegexLineParser* parser) : _parser(parser) {}

    bool isMatch(const std::vector<std::string>& lines, [[maybe_unused]] std::string_view fileName) override {
        if (lines.empty())
            return false;
        std::vector<std::string> columns;
        return _parser->parseLine(lines.front(), columns, *_parser->createContext());
    }
};

class LuaLogDetector : public ILogDetector {
    RegexLineParser* _parser;
    std::string _script;
public:
    LuaLogDetector(RegexLineParser* parser, std::string script) : _parser(parser), _script(script) {
        LuaThread thread;
        if (!thread.pushScript(script))
            throw std::runtime_error("lua script syntax error");
    }

    bool isMatch(const std::vector<std::string>& lines, std::string_view fileName) override {
        LuaThread thread;
        LuaTable luaLines;
        std::vector<std::string> columns;
        for (auto i = 0u; i < lines.size(); ++i) {
            auto luaLine = std::make_shared<LuaTable>();
            luaLine->insert(std::make_shared<LuaString>("text"),
                            std::make_shared<LuaString>(lines[i]));
            luaLine->insert(std::make_shared<LuaString>("parsed"),
                            std::make_shared<LuaBool>(_parser->parseLine(lines[i], columns, *_parser->createContext())));
            luaLines.insert(std::make_shared<LuaInt>(i), luaLine);
        }
        thread.setGlobal("lines", luaLines);
        LuaString luaFileName{std::string(fileName)};
        thread.setGlobal("fileName", luaFileName);
        thread.pushScript(_script);
        if (!thread.execTop())
            return false;
        return thread.popBool();
    }
};

RegexLineParser::RegexLineParser(std::string name) : _name(name) {}

std::string reErrorToString(int code) {
    std::string error(1<<10, 0);
    pcre2_get_error_message(code, (PCRE2_UCHAR*)&error[0], error.size());
    return error;
}

void RegexLineParser::load(std::string config) {
    std::string rePattern;

    try {
        std::stringstream ss{config};
        json j;
        ss >> j;
        auto description = j["description"].get<std::string>();
        rePattern = j["regex"].get<std::string>();

        auto& columns = j["columns"];
        auto magic = j["magic"];
        auto detector = j["detector"];

        if (!magic.is_null() && !detector.is_null())
            throw OptionInconsistencyException("Both 'magic' and 'detector' can't be set at the same time");

        if (!magic.is_null()) {
            _detector = std::make_shared<MagicLogDetector>(magic.get<std::string>());
        } else if (!detector.is_null()) {
            std::string text;
            for (auto it = begin(detector); it != end(detector); ++it) {
                text += it->get<std::string>();
                text += "\n";
            }
            _detector = std::make_shared<LuaLogDetector>(this, text);
        } else {
            _detector = std::make_shared<DefaultLogDetector>(this);
        }

        for (auto it = begin(columns); it != end(columns); ++it) {
            auto name = (*it)["name"].get<std::string>();
            auto group = (*it)["group"].get<int>();
            auto indexed = it->value("indexed", false);
            auto autosize = it->value("autosize", false);
            _formats.push_back({name, group, indexed, autosize});
        }

        auto colors = j["colors"];
        if (!colors.is_null()) {
            for (auto it = begin(colors); it != end(colors); ++it) {
                auto columnName = (*it)["column"].get<std::string>();
                auto value = (*it)["value"].get<std::string>();
                auto color = std::stoi((*it)["color"].get<std::string>(), 0, 16);
                auto column = std::find_if(begin(_formats), end(_formats), [&] (auto& format) {
                    return format.name == columnName;
                });
                if (column == end(_formats))
                    continue;
                auto columnIndex = static_cast<int>(std::distance(begin(_formats), column));
                _colors.push_back({columnIndex, value, static_cast<uint32_t>(color)});
            }
        }
    } catch (json::exception& e) {
        throw JsonParserException(e.what());
    }

    int reError = 0;
    PCRE2_SIZE errorOffset;
    _re.reset(pcre2_compile((PCRE2_SPTR8)rePattern.c_str(),
                            PCRE2_ZERO_TERMINATED,
                            0,
                            &reError,
                            &errorOffset,
                            nullptr),
              pcre2_code_free_8);

    if (!_re)
        throw RegexpSyntaxException(reErrorToString(reError));

    reError = pcre2_jit_compile(_re.get(), PCRE2_JIT_COMPLETE);

    if (reError)
        throw RegexpSyntaxException(reErrorToString(reError));

    auto matchData = std::shared_ptr<pcre2_match_data>(
        pcre2_match_data_create_from_pattern(_re.get(), nullptr),
        pcre2_match_data_free);

    int groupCount = pcre2_get_ovector_count(matchData.get());
    for (auto& format : _formats) {
        if (format.group < 0 || format.group >= groupCount)
            throw RegexpOutOfBoundGroupReferenceException(fmt::format(
                "Column \"{}\" references a nonexistent group \"{}\" (there are only {} groups in the regex).",
                format.name,
                format.group,
                groupCount));
    }
}

bool RegexLineParser::parseLine(std::string_view line,
                                std::vector<std::string>& columns,
                                ILineParserContext& context) {

    auto typedContext = dynamic_cast<RegexLineParserContext*>(&context);
    assert(typedContext);
    auto matchData = typedContext->matchData.get();

    auto rc = pcre2_jit_match(_re.get(),
                              (PCRE2_SPTR8)line.data(),
                              line.size(),
                              0,
                              0,
                              matchData,
                              nullptr);
    if (rc < 0)
        return false;

    auto vec = pcre2_get_ovector_pointer(matchData);
    columns.resize(_formats.size());

    int c = 0;
    for (auto& format : _formats) {
        auto i = format.group;
        assert(static_cast<size_t>(i) < pcre2_get_ovector_count(matchData));
        auto group = line.data() + vec[2 * i];
        auto len = vec[2 * i + 1] - vec[2 * i];
        columns[c].assign(group, len);
        ++c;
    }
    return true;
}

std::vector<ColumnFormat> RegexLineParser::getColumnFormats() {
    std::vector<ColumnFormat> formats;
    for (auto& format : _formats) {
        formats.push_back({format.name, format.indexed, format.autosize});
    }
    return formats;
}

uint32_t RegexLineParser::rgb(const std::vector<std::string>& columns) const {
    for (auto& color : _colors) {
        if (color.column < static_cast<int>(columns.size()) &&
            color.value == columns[color.column])
            return color.color;
    }
    return 0;
}

bool RegexLineParser::isMatch(const std::vector<std::string>& sample,
                              std::string_view fileName) {
    return _detector->isMatch(sample, fileName);
}

std::unique_ptr<ILineParserContext> RegexLineParser::createContext() const {
    auto context = std::make_unique<RegexLineParserContext>();
    context->matchData = std::shared_ptr<pcre2_match_data>(
        pcre2_match_data_create_from_pattern(_re.get(), nullptr),
        pcre2_match_data_free);
    return context;
}

std::string RegexLineParser::name() const{
    return _name;
}

} // namespace seer
