#pragma once

#include "seer/Searcher.h"
#include "gui/grid/LruCache.h"
#include "gui/grid/RowColumn.h"

#include <boost/icl/interval_set.hpp>

namespace gui::grid {

class CachedHighlightSearcher {
    using Set = boost::icl::interval_set<uint64_t>;

    struct Entry {
        Set set;
#ifndef NDEBUG
        QString text;
#endif
    };
    std::unique_ptr<seer::IHighlightSearcher> _searcher;
    LruCache<RowColumn, std::shared_ptr<Entry>, RowColumnHash> _cache;

public:
    explicit CachedHighlightSearcher(std::unique_ptr<seer::IHighlightSearcher> searcher,
                                     size_t cacheSize);
    std::tuple<int, int> search(QString text, size_t start, int row, int column);
    void invalidateCache();
};

} // namespace gui
