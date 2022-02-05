#include <catch2/catch.hpp>

#include <gui/ForeachRange.h>

TEST_CASE("foreach_range_empty") {
    std::vector<int> vec;
    std::vector<std::tuple<int, int>> ranges;
    foreachRange(vec, [&] (int start, int len) {
        ranges.push_back({start, len});
    });
    REQUIRE( ranges.empty() );
}

TEST_CASE("foreach_range_single") {
    std::vector<int> vec { 3, 3, 3 };
    std::vector<std::pair<int, int>> ranges, expected {
        {0, 3}
    };
    foreachRange(vec, [&] (int start, int len) {
        ranges.push_back({start, len});
    });
    REQUIRE( ranges == expected );
}

TEST_CASE("foreach_range_triple") {
    std::vector<int> vec { 0, 1, 1, 1, 0, 0, 0 };
    std::vector<std::pair<int, int>> ranges, expected {
        {0, 1}, {1, 3}, {4, 3}
    };
    foreachRange(vec, [&] (int start, int len) {
        ranges.push_back({start, len});
    });
    REQUIRE( ranges == expected );
}
