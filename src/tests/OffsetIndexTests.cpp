#include <catch2/catch.hpp>

#include "seer/OffsetIndex.h"
#include <ranges>

TEST_CASE("offset_index") {
    std::vector<uint64_t> offsets {
        0, 3, 7, 18, 20, 21, 22, 30, 70, 101, 103
    };

    seer::OffsetIndex index;
    for (auto delta : {2, 4, 8, 16, 32, 64}) {
        index.reset(delta, [&](uint64_t offset) {
            auto it = std::ranges::find(offsets, offset);
            return it[1];
        });

        REQUIRE( index.size() == 0 );

        for (auto offset : offsets) {
            index.add(offset);
        }

        REQUIRE( index.size() == offsets.size() );

        for (auto i = 0u; i < offsets.size(); ++i) {
            REQUIRE( index.map(i) == offsets[i] );
        }
    }
}
