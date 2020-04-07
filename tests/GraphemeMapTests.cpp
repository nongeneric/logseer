#include <catch2/catch.hpp>

#include <gui/GraphemeMap.h>

using namespace gui;

TEST_CASE("grapheme_map_simple") {
    QString line{"1"};
    GraphemeMap gmap(line);
    REQUIRE( gmap.graphemeSize() == 1 );
    REQUIRE( gmap.toVisibleRange(0, 0) == std::tuple(0, 0) );
    REQUIRE( gmap.indexToGraphemeRange(0) == std::tuple(0, 0) );
    REQUIRE( gmap.graphemeToIndexRange(0) == std::tuple(0, 0) );
}

TEST_CASE("grapheme_map_tab") {
    // 1___2___3 (tab 4)
    // 012345678
    QString line{"1\t2\t3"};
    GraphemeMap gmap(line);
    REQUIRE( gmap.graphemeSize() == 9 );
    REQUIRE( gmap.toVisibleRange(0, 2) == std::tuple(0, 3) );
    REQUIRE( gmap.toVisibleRange(0, 3) == std::tuple(0, 3) );
    REQUIRE( gmap.toVisibleRange(0, 4) == std::tuple(0, 4) );
    REQUIRE( gmap.toVisibleRange(0, 5) == std::tuple(0, 7) );
    REQUIRE( gmap.toVisibleRange(1, 5) == std::tuple(1, 7) );
    REQUIRE( gmap.indexToGraphemeRange(0) == std::tuple(0, 0) );
    REQUIRE( gmap.indexToGraphemeRange(1) == std::tuple(1, 3) );
    REQUIRE( gmap.indexToGraphemeRange(2) == std::tuple(4, 4) );
    REQUIRE( gmap.indexToGraphemeRange(3) == std::tuple(5, 7) );
    REQUIRE( gmap.graphemeToIndexRange(2) == std::tuple(1, 1) );
    REQUIRE( gmap.graphemeToIndexRange(3) == std::tuple(1, 1) );
    REQUIRE( gmap.graphemeToIndexRange(4) == std::tuple(2, 2) );
    REQUIRE( gmap.graphemeRangeToString(0, 0) == "1" );
    REQUIRE( gmap.graphemeRangeToString(0, 2) == "1  " );
    REQUIRE( gmap.graphemeRangeToString(2, 5) == "  2 " );
}

TEST_CASE("grapheme_map_grapheme_cluster") {
    QString line{u8"g̈"}; // 0067 + 0308
    REQUIRE( line.size() == 2 );
    GraphemeMap gmap(line);
    REQUIRE( gmap.graphemeSize() == 1 );
    REQUIRE( gmap.toVisibleRange(0, 0) == std::tuple(0, 0) );
    REQUIRE( gmap.indexToGraphemeRange(0) == std::tuple(0, 0) );
    REQUIRE( gmap.graphemeToIndexRange(0) == std::tuple(0, 1) );
    REQUIRE( gmap.graphemeRangeToString(0, 0) == line );
}

TEST_CASE("grapheme_map_grapheme_cluster_and_tab") {
    QString line{u8"g̈1\t2g̈3"}; // 0067 + 0308
    // index: gg1t2gg3
    // graph: g1__2g3
    //        01234567
    REQUIRE( line.size() == 8 );
    GraphemeMap gmap(line);
    REQUIRE( gmap.graphemeSize() == 7 );
    REQUIRE( gmap.toVisibleRange(3, 3) == std::tuple(2, 3) );
    REQUIRE( gmap.indexToGraphemeRange(3) == std::tuple(2, 3) );
    REQUIRE( gmap.indexToGraphemeRange(6) == std::tuple(5, 5) );
    REQUIRE( gmap.graphemeToIndexRange(0) == std::tuple(0, 1) );
    REQUIRE( gmap.graphemeToIndexRange(2) == std::tuple(3, 3) );
    REQUIRE( gmap.graphemeToIndexRange(3) == std::tuple(3, 3) );
    REQUIRE( gmap.graphemeToIndexRange(5) == std::tuple(5, 6) );
}

TEST_CASE("grapheme_map_visible_range_words_simple") {
    QString line{"some words here"};
    GraphemeMap gmap(line);
    REQUIRE( gmap.toVisibleRange(0, 0, VisibleRangeType::Word) == std::tuple(0, 3) );
    REQUIRE( gmap.toVisibleRange(7, 7, VisibleRangeType::Word) == std::tuple(5, 9) );
    REQUIRE( gmap.toVisibleRange(2, 7, VisibleRangeType::Word) == std::tuple(0, 9) );
    REQUIRE( gmap.toVisibleRange(2, 12, VisibleRangeType::Word) == std::tuple(0, 14) );
}

TEST_CASE("grapheme_map_empty_line") {
    QString line{""};
    GraphemeMap gmap(line);
    REQUIRE( gmap.toVisibleRange(-1, -1, VisibleRangeType::Word) == std::tuple(-1, -1) );
    REQUIRE( gmap.graphemeSize() == 0 );
}

TEST_CASE("grapheme_map_unicode_and_tab") {
    QString line{u8"\tfürfür"};
    // graph: ____fürfür
    //        0123456789
    GraphemeMap gmap(line);
    REQUIRE( gmap.graphemeSize() == 10 );
    REQUIRE( gmap.toVisibleRange(2, 7) == std::tuple(0, 7) );
    REQUIRE( gmap.toVisibleRange(2, 100) == std::tuple(0, 9) );
    REQUIRE( gmap.toVisibleRange(8, 9) == std::tuple(8, 9) );
}
