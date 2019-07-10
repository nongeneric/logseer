#include <catch2/catch.hpp>

#include "seer/Searcher.h"

using namespace seer;

TEST_CASE("searcher_regex_dont_search_past_last_char") {
    auto searcher = createSearcher("$$$$", true, false);
    auto [first, len] = searcher->search("123", 10);
    REQUIRE( first == -1 );
    std::tie(first, len) = searcher->search("123", 0);
    REQUIRE( first == -1 );
}
