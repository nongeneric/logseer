#include <catch2/catch.hpp>

#include "seer/RandomBitArray.h"
#include <vector>
#include <limits>

TEST_CASE("random_bit_array") {
    std::vector<uint64_t> vec;
    for (int bucketSize : {2, 4, 8, 16, 32, 64, 128, 1024}) {
        seer::RandomBitArray rba(bucketSize);
        constexpr int max = 2000;
        for (int step : {1, 3, 7, 13, 29}) {
            rba.clear();
            vec.clear();
            for (int i = 0; i < max; i += step) {
                vec.push_back(i);
                rba.add(i);
            }
            for (auto i = 0u; i < vec.size(); ++i) {
                REQUIRE( vec[i] == rba.get(i) );
            }
        }
    }
}
