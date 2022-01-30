#include "FilterAlgo.h"

namespace seer {

FilterAlgo::FilterAlgo(const ewah_bitset& baseSet,
                       std::vector<const ewah_bitset*>& baseVec,
                       std::vector<const ewah_bitset*>& newVec)
    : _baseSet(baseSet), _newVec(newVec)
{
    std::sort(begin(baseVec), end(baseVec));
    std::sort(begin(newVec), end(newVec));
    std::set_difference(begin(newVec), end(newVec), begin(baseVec), end(baseVec), std::back_inserter(_added));
    std::set_difference(begin(baseVec), end(baseVec), begin(newVec), end(newVec), std::back_inserter(_removed));
}

FilterAlgoStats FilterAlgo::stats() const {
    return {.naiveOps = _newVec.size(), .diffOps = 2 + _added.size() + _removed.size()};
}

ewah_bitset FilterAlgo::naive() {
    return fast_logicalor(_newVec.size(), &_newVec[0]);
}

ewah_bitset FilterAlgo::diff() {
    auto addedSet = fast_logicalor(_added.size(), &_added[0]);
    auto removedSet = fast_logicalor(_removed.size(), &_removed[0]);
    return (_baseSet | addedSet) - removedSet;
}

} // namespace seer
