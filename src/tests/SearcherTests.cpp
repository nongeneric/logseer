#include <catch2/catch.hpp>

#include "seer/StringLiterals.h"
#include "seer/Searcher.h"

#include "gui/grid/CachedHighlightSearcher.h"

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

TEST_CASE("cached_highlight_searcher") {
    auto searcher = createHighlightSearcher("\\d\\d", true, false, false);
    gui::grid::CachedHighlightSearcher cachedSearcher(std::move(searcher), 100);
    QString text = "11aa22aa33bbcc44dd";

    {
        auto [first, len] = cachedSearcher.search(text, 0, 0, 0);
        REQUIRE( first == 0 );
        REQUIRE( len == 2 );
    }

    {
        auto [first, len] = cachedSearcher.search(text, 1, 0, 0);
        REQUIRE( first == 1 );
        REQUIRE( len == 1 );
    }

    {
        auto [first, len] = cachedSearcher.search(text, 2, 0, 0);
        REQUIRE( first == 4 );
        REQUIRE( len == 2 );
    }

    {
        auto [first, len] = cachedSearcher.search(text, text.size(), 0, 0);
        REQUIRE( first == -1 );
        REQUIRE( len == -1 );
    }

    {
        auto [first, len] = cachedSearcher.search(text, 100, 0, 0);
        REQUIRE( first == -1 );
        REQUIRE( len == -1 );
    }
}

TEST_CASE("cached_highlight_searcher_empty_length_regex_result") {
    auto searcher = createHighlightSearcher("|||", true, false, false);
    gui::grid::CachedHighlightSearcher cachedSearcher(std::move(searcher), 100);
    QString text = "11aa22aa33bbcc44dd";

    {
        auto [first, len] = cachedSearcher.search(text, 0, 0, 0);
        REQUIRE( first == -1 );
        REQUIRE( len == -1 );
    }
}

