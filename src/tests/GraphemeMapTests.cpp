#include <catch2/catch.hpp>

#include <fmt/format.h>
#include <seer/StriingLiterals.h>
#include <gui/GraphemeMap.h>

using namespace gui;

class TestMetrics : public IFontMetrics {
public:
    float width(const QString&) override {
        return 1;
    }
};

TestMetrics g_singleSizeTestMetrics;

TEST_CASE("grapheme_map_simple") {
    QString line{"1"};
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.graphemeSize() == 1 );
    REQUIRE( gmap.getPosition(0) == 0.f );
    REQUIRE( gmap.getPosition(1) == 1.f );
    REQUIRE( gmap.indexToGrapheme(0) == 0 );
}

TEST_CASE("grapheme_map_tab") {
    // text: 1t2t3
    // pos : 1___2___3 (tab 4)
    //       012345678
    QString line{"1\t2\t3"};
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.graphemeSize() == 5 );
    REQUIRE( gmap.getPosition(0) == 0.f );
    REQUIRE( gmap.getPosition(1) == 1.f );
    REQUIRE( gmap.getPosition(2) == 4.f );
    REQUIRE( gmap.getPosition(3) == 5.f );
    REQUIRE( gmap.getPosition(4) == 8.f );
    REQUIRE( gmap.findGrapheme(0.f) == 0 );
    REQUIRE( gmap.findGrapheme(0.7f) == 0 );
    REQUIRE( gmap.findGrapheme(3.2f) == 1 );
    REQUIRE( gmap.findGrapheme(6.8f) == 3 );
}

TEST_CASE("grapheme_map_grapheme_cluster") {
    QString line{u8"g̈"_as_char}; // 0067 + 0308
    REQUIRE( line.size() == 2 );
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.graphemeSize() == 1 );
    REQUIRE( gmap.getPosition(0) == 0.f );
    REQUIRE( gmap.indexToGrapheme(0) == 0 );
    REQUIRE( gmap.indexToGrapheme(1) == 0 );
}

TEST_CASE("grapheme_map_grapheme_cluster_and_tab") {
    QString line{u8"g̈1\t2g̈3"_as_char}; // 0067 + 0308
    // index: gg1t2gg3
    // graph: g1t2g3
    // pos  : g1__2g3 (tab 4)
    //        0123456
    REQUIRE( line.size() == 8 );
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.graphemeSize() == 6 );
    REQUIRE( gmap.getPosition(2) == 2.f );
    REQUIRE( gmap.getPosition(3) == 4.f );
}

TEST_CASE("grapheme_map_visible_range_words_simple") {
    QString line{"some words here"};
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.extendToWordBoundary(0, 0) == std::tuple(0, 3) );
    REQUIRE( gmap.extendToWordBoundary(7, 7) == std::tuple(5, 9) );
    REQUIRE( gmap.extendToWordBoundary(2, 7) == std::tuple(0, 9) );
    REQUIRE( gmap.extendToWordBoundary(2, 12) == std::tuple(0, 14) );
}

TEST_CASE("grapheme_map_empty_line") {
    QString line{""};
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.extendToWordBoundary(-1, -1) == std::tuple(-1, -1) );
    REQUIRE( gmap.extendToWordBoundary(2, 3) == std::tuple(-1, -1) );
    REQUIRE( gmap.graphemeSize() == 0 );
    REQUIRE( gmap.findGrapheme(0.f) == -1 );
    REQUIRE( gmap.pixelWidth() == 0 );
}

TEST_CASE("grapheme_map_unicode_and_tab") {
    QString line{u8"\tfürfür"_as_char};
    // graph: tfürfür
    // pos  : ____furfur (4 tab)
    //        0123456789
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.graphemeSize() == 7 );
    REQUIRE( gmap.getPosition(0) == 0.f );
    REQUIRE( gmap.getPosition(1) == 4.f );
    REQUIRE( gmap.getPosition(2) == 5.f );
    REQUIRE( gmap.pixelWidth() == 10 );
}

TEST_CASE("grapheme_map_negative_position") {
    QString line{"123"};
    GraphemeMap gmap(line, &g_singleSizeTestMetrics);
    REQUIRE( gmap.graphemeSize() == 3 );
    REQUIRE( gmap.findGrapheme(-100) == 0 );
}
