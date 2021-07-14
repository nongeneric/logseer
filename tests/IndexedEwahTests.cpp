#include <catch2/catch.hpp>

#include "seer/IndexedEwah.h"

TEST_CASE("indexed_ewah_simple") {
    ewah::EWAHBoolArray<uint64_t> array;
    std::vector<uint32_t> values {
        0, 3, 4, 5, 7, 10, 105, 500
    };

    for (auto v : values) {
        array.set(v);
    }

    seer::IndexedEwah iewah(50);
    iewah.init(array);
    REQUIRE( iewah.size() == values.size() );
    for (size_t i = 0; i < values.size(); ++i) {
        REQUIRE( iewah.get(i) == values[i] );
    }
}
