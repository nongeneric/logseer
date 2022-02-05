#pragma once

#include "IRandomArray.h"
#include <vector>
#include <ewah/ewah.h>

namespace seer {

class RandomBitArray : public IRandomArray {
    unsigned _bucketSize;
    uint64_t _lastValue = 0;
    unsigned _currentBucketSize = -1;
    std::vector<ewah::EWAHBoolArray<uint64_t>> _buckets;

public:
    RandomBitArray(unsigned bucketSize);
    void add(uint64_t value);
    uint64_t get(uint64_t index) override;
    uint64_t size() const override;
    void clear();
};

} // namespace seer
