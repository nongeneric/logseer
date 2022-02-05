#include <catch2/catch.hpp>

#include "seer/IndexedEwah.h"
#include <random>

TEST_CASE("indexed_ewah_empty_array") {
    ewah::EWAHBoolArray<uint64_t> array;
    seer::IndexedEwah iewah(64);
    iewah.init(array);
    REQUIRE( iewah.size() == 0 );
}

TEST_CASE("indexed_ewah_single_element") {
    ewah::EWAHBoolArray<uint64_t> array;
    array.set(105);

    seer::IndexedEwah iewah(64);
    iewah.init(array);

    REQUIRE( iewah.size() == 1 );
    REQUIRE( iewah.get(0) == 105 );
}

TEST_CASE("indexed_ewah_simple") {
    ewah::EWAHBoolArray<uint64_t> array;
    std::vector<uint32_t> values {
        0, 3, 4, 5, 7, 10, 63, 64, 65, 105, 126, 127, 128, 500
    };

    for (auto v : values) {
        array.set(v);
    }

    seer::IndexedEwah iewah(128);
    iewah.init(array);
    REQUIRE( iewah.size() == values.size() );
    for (size_t i = 0; i < values.size(); ++i) {
        REQUIRE( iewah.get(i) == values[i] );
    }
}

TEST_CASE("indexed_ewah_random") {
    ewah::EWAHBoolArray<uint64_t> array;

    std::mt19937 g(1);
    for (int i = 0; i < 100000;) {
        array.set(i);
        i += g() % 5;
    }

    auto values = array.toArray();

    seer::IndexedEwah iewah(256);
    iewah.init(array);
    REQUIRE( iewah.size() == values.size() );
    for (size_t i = 0; i < values.size(); ++i) {
        REQUIRE( iewah.get(i) == values[i] );
    }
}
