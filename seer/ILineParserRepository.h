#pragma once

#include "ILineParser.h"
#include <istream>
#include <memory>

namespace seer {

    class ILineParserRepository {
    public:
        virtual std::shared_ptr<ILineParser> resolve(std::istream& stream) = 0;
        virtual ~ILineParserRepository() = default;
    };

} // namespace seer
