#include "gui/grid/CachedHighlightSearcher.h"

namespace gui::grid {

CachedHighlightSearcher::CachedHighlightSearcher(std::unique_ptr<seer::IHighlightSearcher> searcher, size_t cacheSize)
    : _searcher(std::move(searcher)), _cache(cacheSize) {}

std::tuple<int, int> CachedHighlightSearcher::search(QString text, size_t start, int row, int column) {
    std::shared_ptr<Entry> entry;
    if (auto it = _cache.lookup({row, column})) {
        entry = it.value();
        assert(text == entry->text);
    } else {
        entry = std::make_shared<Entry>();
        int index = 0;
        for (;;) {
            auto [start, len] = _searcher->search(text, index);
            if (start == -1 || len == 0)
                break;
            entry->set.insert(Set::interval_type::right_open(start, start + len));
            index = start + len;
        }
#ifndef NDEBUG
        entry->text = text;
#endif
        _cache.insert({row, column}, entry);
    }
    if (start >= static_cast<size_t>(text.size()))
        return {-1, -1};
    if (auto it = entry->set.lower_bound(Set::interval_type::right_open(start, text.size())); it != entry->set.end()) {
        start = std::max(start, it->lower());
        return {start, it->upper() - start};
    }
    return {-1, -1};
}

void CachedHighlightSearcher::invalidateCache() {
    _cache.clear();
}

} // namespace gui::grid
