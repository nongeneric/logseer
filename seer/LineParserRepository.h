#pragma once

#include "ILineParserRepository.h"

namespace seer {

    class LineParserRepository : public ILineParserRepository {
    public:
        std::shared_ptr<ILineParser> resolve(std::istream &stream) override;
    };

} // namespace seer
