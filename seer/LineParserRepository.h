#pragma once

#include "ILineParserRepository.h"
#include <map>

namespace seer {

    class LineParserRepository : public ILineParserRepository {
        std::map<int, std::shared_ptr<ILineParser>> _parsers;

    public:
        void init();
        std::shared_ptr<ILineParser> resolve(std::istream &stream) override;
    };

} // namespace seer
