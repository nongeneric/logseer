#pragma once

#include "FileParser.h"
#include "ILineParser.h"
#include "RandomBitArray.h"
#include <ewah.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <tuple>
#include <set>
#include <experimental/ranges/range>

namespace seer {

    using ewah_bitset = EWAHBoolArray<uint64_t>;

    struct ColumnIndexInfo {
        std::string value;
        bool checked;
        uint64_t count;
    };

    struct ColumnFilter {
        int column;
        std::set<std::string> selected;
    };

    struct ColumnInfo {
        std::unordered_map<std::string, ewah_bitset> index;
        bool indexed = false;
        int maxWidth = -1;
    };

    class Index {
        RandomBitArray _lineMap;
        std::vector<ColumnInfo> _columns;
        uint64_t _unfilteredLineCount = 0;
        bool _filtered = false;
        ewah_bitset _filter;
        std::vector<ColumnFilter> _filters;
        ewah_bitset makeIndex(std::vector<ColumnFilter>::const_iterator first,
                              std::vector<ColumnFilter>::const_iterator last);

    public:
        Index(uint64_t unfilteredLineCount = 0);
        void filter(const std::vector<ColumnFilter>& filters);
        void search(FileParser* fileParser, std::string text, bool caseSensitive);
        uint64_t getLineCount();
        uint64_t mapIndex(uint64_t index);
        bool index(FileParser* fileParser,
                   ILineParser* lineParser,
                   std::function<bool()> stopRequested,
                   std::function<void(uint64_t, uint64_t)> progress = {});
        std::vector<ColumnIndexInfo> getValues(int column);
        int maxWidth(int column);
    };

} // namespace seer
