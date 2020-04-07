#pragma once

#include <algorithm>

template <class F>
void foreachRange(const std::vector<int>& vec, F action) {
    if (vec.empty())
        return;
    auto it = begin(vec);
    do {
        auto next = std::find_if(it, end(vec), [&](int val) { return val != *it; });
        action(std::distance(begin(vec), it), std::distance(it, next));
        it = next;
    } while(it != end(vec));
}
