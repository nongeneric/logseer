#include <catch2/catch.hpp>

#include "seer/StriingLiterals.h"
#include "seer/Searcher.h"

using namespace seer;

TEST_CASE("searcher_regex_dont_search_past_last_char") {
    auto searcher = createHighlightSearcher("$$$$", true, false, false);
    auto [first, len] = searcher->search("123", 10);
    REQUIRE( first == -1 );
    std::tie(first, len) = searcher->search("123", 0);
    REQUIRE( first == -1 );
}

TEST_CASE("highlight_searcher_unicode_16") {
    auto searcher = createHighlightSearcher("b", true, false, true);
    auto [first, len] = searcher->search(u8"@1ạb|b"_as_char, 0);
    REQUIRE( first == 4 );
    REQUIRE( u"@1ạb|b"[first] == 'b' );
}

TEST_CASE("highlight_searcher_unicode_8") {
    auto searcher = createSearcher("b", true, false, true);
    auto [first, len] = searcher->search(u8"@1ạb|b"_as_char, 0);
    REQUIRE( first == 5 );
    REQUIRE( u8"@1ạb|b"[first] == 'b' );
}
