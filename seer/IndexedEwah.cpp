#include "IndexedEwah.h"

namespace seer {

IndexedEwah::IndexedEwah(unsigned bucketSize)
    : _bucketSize(bucketSize)
{ }

void IndexedEwah::init(const ewah::EWAHBoolArray<uint64_t> &ewah)
{
    auto it = ewah.begin();
    auto end = ewah.end();
    auto i = 0;
    while (it != end) {
        if (i % _bucketSize == 0) {
            _buckets.push_back(it);
        }
        ++it;
        ++i;
    }
    _size = i;
}

uint64_t IndexedEwah::get(uint64_t index) {
    auto it = _buckets[index / _bucketSize];
    auto offset = index % _bucketSize;
    for (auto i = 0ull; i < offset; ++i, ++it) ;
    return *it;
}

uint64_t IndexedEwah::size() const {
    return _size;
}

} // namespace seer
