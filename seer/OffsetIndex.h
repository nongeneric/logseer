#pragma once

#include <vector>
#include <functional>
#include "stdint.h"

namespace seer {

class OffsetIndex {
    using NextCallback = std::function<uint64_t(uint64_t)>;
    std::vector<uint64_t> _index;
    int _delta;
    NextCallback _next;
    uint64_t _current = 0;

public:
    void add(uint64_t value);
    uint64_t map(uint64_t index);
    void reset(int delta, NextCallback next);
    uint64_t size();
};

} // namespace seer
