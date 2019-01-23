#pragma once

#include "ILineParserRepository.h"
#include <map>

namespace seer {

    class LineParserRepository : public ILineParserRepository {
        std::map<int, std::shared_ptr<ILineParser>> _parsers;

    public:
        void addRegexParser(std::string name, int priority, std::string json);
        std::shared_ptr<ILineParser> resolve(std::istream &stream) override;
    };

} // namespace seer
