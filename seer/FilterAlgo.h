#pragma once

#include <ewah/ewah.h>
#include <vector>

using ewah_bitset = ewah::EWAHBoolArray<uint64_t>;

namespace seer {

struct FilterAlgoStats {
    size_t naiveOps{};
    size_t diffOps{};
};

class FilterAlgo {
    const ewah_bitset& _baseSet;
    std::vector<const ewah_bitset*>& _baseVec;
    std::vector<const ewah_bitset*>& _newVec;
    std::vector<const ewah_bitset*> _removed;
    std::vector<const ewah_bitset*> _added;

public:
    FilterAlgo(const ewah_bitset& baseSet,
               std::vector<const ewah_bitset*>& baseVec,
               std::vector<const ewah_bitset*>& newVec);

    FilterAlgoStats stats() const;
    ewah_bitset naive();
    ewah_bitset diff();
};

} // namespace seer
