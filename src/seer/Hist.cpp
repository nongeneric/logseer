#include "Hist.h"

#include <assert.h>

namespace {

int scale(int n, int size, int newSize) {
    return n / float(size) * newSize;
}

}

namespace seer {

Hist::Hist(int size) : _hist(size) {}

void Hist::add(int n, int count) {
    _hist[scale(n, count, _hist.size())].fetch_add(1, std::memory_order_relaxed);
}

int Hist::get(int n, int count) const {
    assert(_frozen);
    auto first = scale(n, count, _hist.size());
    auto last = std::min(scale(n + 1, count, _hist.size()) - 1, (int)_hist.size() - 1);
    int sum = 0;
    for (int i = first; i <= last; ++i) {
        sum += _hist[i];
    }
    return sum;
}

void Hist::freeze() {
    _frozen = true;
}

} // namespace seer
