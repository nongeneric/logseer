#pragma once

#include <vector>
#include <atomic>
#include <stdint.h>

namespace seer {

    class Hist {
        std::vector<std::atomic<int32_t>> _hist;
        std::atomic<bool> _frozen = false;

    public:
        Hist(int size);
        void add(int n, int count);
        int get(int n, int count) const;
        void freeze();
    };

} // namespace seer
