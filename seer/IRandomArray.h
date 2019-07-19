#pragma once

#include "stdint.h"

namespace seer {

    class IRandomArray {
    public:
        virtual ~IRandomArray() = default;
        virtual uint64_t get(uint64_t index) = 0;
        virtual uint64_t size() const = 0;
    };

} // namespace seer
