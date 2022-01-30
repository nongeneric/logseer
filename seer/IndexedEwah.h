#pragma once

#include "IRandomArray.h"
#include <ewah/ewah.h>
#include <vector>

namespace seer {

class IndexedEwah : public IRandomArray {
    using Iter = ewah::EWAHBoolArrayIterator<uint64_t>;

    struct Bucket {
        uint64_t firstIndex;
        Iter iter;
    };

    unsigned _bucketSize;
    std::vector<Bucket> _buckets;
    uint64_t _size = 0;

public:
    IndexedEwah(unsigned bucketSize);
    void init(ewah::EWAHBoolArray<uint64_t> const& ewah);
    uint64_t get(uint64_t index) override;
    uint64_t size() const override;
    size_t sizeInBytes() const;
};

} // namespace seer
