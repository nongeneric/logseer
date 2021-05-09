#pragma once

#include "IRandomArray.h"
#include <ewah.h>
#include <vector>

namespace seer {

class IndexedEwah : public IRandomArray {
    using Iter = decltype(((EWAHBoolArray<uint64_t>*)1)->begin());
    unsigned _bucketSize;
    std::vector<Iter> _buckets;
    int _size = 0;

public:
    IndexedEwah(unsigned bucketSize);
    void init(EWAHBoolArray<uint64_t> const& ewah);
    uint64_t get(uint64_t index) override;
    uint64_t size() const override;
};

} // namespace seer
