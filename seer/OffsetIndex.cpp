#include "OffsetIndex.h"

#include "assert.h"

namespace seer {

void OffsetIndex::add(uint64_t value) {
    if (_current % _delta == 0) {
        _index.push_back(value);
    }
    _current++;
}

uint64_t OffsetIndex::map(uint64_t index) {
    auto lower = index & ~(_delta - 1);
    assert(lower / _delta < _index.size());
    auto offset = _index[lower / _delta];
    while (lower != index) {
        offset = _next(offset);
        lower++;
    }
    return offset;
}

void OffsetIndex::reset(int delta, OffsetIndex::NextCallback next) {
    assert(delta > 0 && (delta & (delta - 1)) == 0);
    _delta = delta;
    _current = 0;
    _next = next;
    _index.clear();
}

uint64_t OffsetIndex::size() {
    return _current;
}

} // namespace seer
