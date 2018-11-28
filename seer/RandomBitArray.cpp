#include "RandomBitArray.h"

namespace seer {

    RandomBitArray::RandomBitArray(unsigned bucketSize) : _bucketSize(bucketSize) { }

    void RandomBitArray::add(uint64_t value) {
        assert(value >= _lastValue);
        _lastValue = value;
        if (_currentBucketSize >= _bucketSize) {
            _buckets.push_back({});
            _currentBucketSize = 0;
        }
        _currentBucketSize++;
        _buckets.back().set(value);
    }

    uint64_t RandomBitArray::get(uint64_t index) {
        assert(index < (_buckets.size() - 1) * _bucketSize + _currentBucketSize);
        auto it = _buckets[index / _bucketSize].begin();
        for (auto i = 0u; i < index % _bucketSize; ++i) {
            ++it;
        }
        return *it;
    }

    uint64_t RandomBitArray::size() const {
        if (_buckets.empty())
            return 0;
        return (_buckets.size() - 1) * _bucketSize + _currentBucketSize;
    }

    void RandomBitArray::clear() {
        _lastValue = 0;
        _buckets.clear();
        _currentBucketSize = -1;
    }

} // namespace seer
