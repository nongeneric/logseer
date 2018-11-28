#pragma once

#include <vector>
#include <ewah.h>

namespace seer {

    class RandomBitArray {
        unsigned _bucketSize;
        uint64_t _lastValue = 0;
        unsigned _currentBucketSize = -1;
        std::vector<EWAHBoolArray<uint64_t>> _buckets;

    public:
        RandomBitArray(unsigned bucketSize);
        void add(uint64_t value);
        uint64_t get(uint64_t index);
        uint64_t size() const;
        void clear();
    };

} // namespace seer
