#pragma once

#include "ILineParser.h"
#include <range/v3/view.hpp>
#include <istream>
#include <memory>
#include <map>

namespace seer {

using ParserMap = std::map<int, std::shared_ptr<ILineParser>>;

class ILineParserRepository {
public:
    virtual std::shared_ptr<ILineParser> resolve(std::istream& stream) = 0;
    virtual const ParserMap& parsers() const = 0;
    virtual ~ILineParserRepository() = default;
};

inline std::shared_ptr<ILineParser> resolveByName(ILineParserRepository* repository, std::string name) {
    for (auto& parser : repository->parsers() | ranges::views::values) {
        if (parser->name() == name)
            return parser;
    }
    throw std::runtime_error("parser not found");
}

} // namespace seer
