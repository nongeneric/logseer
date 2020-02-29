#pragma once

#include "ILineParserRepository.h"
#include <map>
#include <optional>

namespace seer {

    class LineParserRepository : public ILineParserRepository {
        ParserMap _parsers;

    public:
        LineParserRepository(bool initializeDefaultParser = true);
        std::optional<std::string> addRegexParser(std::string name, int priority, std::string json);
        std::shared_ptr<ILineParser> resolve(std::istream &stream) override;
        const ParserMap& parsers() const;
    };

} // namespace seer
