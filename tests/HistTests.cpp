#include <catch2/catch.hpp>

#include "seer/Hist.h"

using namespace seer;

TEST_CASE("hist_simple") {
    Hist hist(100);
    hist.add(0, 100);
    hist.add(99, 100);
    hist.freeze();
    REQUIRE(hist.get(0, 100) == 1);
    REQUIRE(hist.get(1, 100) == 0);
    REQUIRE(hist.get(98, 100) == 0);
    REQUIRE(hist.get(99, 100) == 1);
    REQUIRE(hist.get(0, 1) == 2);
    REQUIRE(hist.get(0, 2) == 1);
    REQUIRE(hist.get(1, 2) == 1);
}

TEST_CASE("hist_simple_2") {
    Hist hist(100);
    hist.add(0, 100);
    hist.add(1, 100);
    hist.add(2, 100);
    hist.add(3, 100);
    hist.add(99, 100);
    hist.freeze();
    REQUIRE(hist.get(0, 50) == 2);
    REQUIRE(hist.get(1, 50) == 2);
    REQUIRE(hist.get(48, 50) == 0);
    REQUIRE(hist.get(49, 50) == 1);
}

TEST_CASE("hist_simple_3") {
    Hist hist(1000);
    hist.add(2, 3);
    hist.freeze();
    REQUIRE(hist.get(0, 3) == 0);
    REQUIRE(hist.get(1, 3) == 0);
    REQUIRE(hist.get(2, 3) == 1);
}
