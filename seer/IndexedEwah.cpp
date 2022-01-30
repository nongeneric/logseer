#include "IndexedEwah.h"
#include <bitset>

namespace seer {

constexpr size_t g_bitsInWord = sizeof(uint64_t) * 8;

IndexedEwah::IndexedEwah(unsigned bucketSize)
    : _bucketSize(bucketSize)
{
    assert(bucketSize >= g_bitsInWord && (bucketSize % g_bitsInWord == 0));
}

void IndexedEwah::init(const ewah::EWAHBoolArray<uint64_t> &ewah) {
    auto it = ewah.uncompress();
    auto bucketSizeInWords = _bucketSize / g_bitsInWord;
    uint64_t words = 0;
    while (it.hasNext()) {
        if (words % bucketSizeInWords == 0) {
            _buckets.push_back({
                .firstIndex = _size,
                .iter = it
            });
        }
        _size += __builtin_popcountll(it.next());
        words++;
    }
    // last bucket may contain one extra word
}

size_t indexOfNthSetBit(uint64_t word, size_t n) {
    assert(static_cast<size_t>(__builtin_popcountll(word)) > n);
    size_t index = 0;
    std::bitset<64> bits(word);
    for (;;) {
        while (!bits[index])
            index++;
        if (n == 0)
            return index;
        n--;
        index++;
    }
    return index;
}

uint64_t IndexedEwah::get(uint64_t index) {
    assert(index < _size);
    auto it = std::lower_bound(
        begin(_buckets), end(_buckets), index, [](const Bucket& a, uint64_t index) {
            return a.firstIndex < index;
        });
    if (it == end(_buckets) || it->firstIndex != index)
        --it;
    uint64_t value = std::distance(begin(_buckets), it) * _bucketSize;
    auto ewahIt = it->iter;
    index -= it->firstIndex;
    for (;;) {
        assert(ewahIt.hasNext());
        auto word = ewahIt.next();
        size_t ones = __builtin_popcountll(word);
        if (ones > index) {
            return value + indexOfNthSetBit(word, index);
        }
        index -= ones;
        value += g_bitsInWord;
    }
    assert(false);
    return 0;
}

uint64_t IndexedEwah::size() const {
    return _size;
}

size_t IndexedEwah::sizeInBytes() const {
    return _buckets.size() * sizeof(Bucket);
}

} // namespace seer
