#include <catch2/catch.hpp>
#include <seer/FilterAlgo.h>
#include <random>
#include <set>
#include <numeric>

using namespace seer;

static auto initVectors(int lineCount, int bitsetCount, int baseCount, int addedCount, int removedCount) {
    std::mt19937 g(1);

    std::vector<ewah_bitset> sets(bitsetCount);
    std::vector<const ewah_bitset*> all;
    std::vector<const ewah_bitset*> base;
    std::vector<const ewah_bitset*> added;
    std::vector<const ewah_bitset*> removed;

    std::vector<int> lines(lineCount);
    std::iota(begin(lines), end(lines), 0);
    std::shuffle(begin(lines), end(lines), g);

    auto line = begin(lines);
    for (auto& set : sets) {
        for (int i = 0; i < lineCount / bitsetCount; ++i) {
            assert(line != end(lines));
            set.set(*line++);
        }
    }

    for (auto& set : sets) {
        all.push_back(&set);
    }

    auto select = [&] (auto& from, auto& to, int size) {
        for (int i = 0; i < size; ++i) {
            assert(!from.empty());
            auto set = begin(from) + (g() % from.size());
            to.push_back(*set);
            from.erase(set);
        }
    };

    select(all, base, baseCount);
    select(all, added, addedCount);
    auto baseCopy = base;
    select(baseCopy, removed, removedCount);

    std::set<const ewah_bitset*> res;
    for (auto set : base)
        res.insert(set);
    for (auto set : added)
        res.insert(set);
    for (auto set : removed)
        res.erase(set);

    auto baseSet = ewah::fast_logicalor(base.size(), &base[0]);
    std::vector<const ewah_bitset*> resVec(begin(res), end(res));

    return std::tuple(std::move(sets), std::move(baseSet), std::move(base), std::move(resVec));
}

TEST_CASE("filter_algo_1") {
    auto [sets, base, basePtrs, newPtrs] = initVectors(10000, 200, 180, 10, 30);
    FilterAlgo algo(base, basePtrs, newPtrs);

    REQUIRE( basePtrs.size() == 180 );
    REQUIRE( newPtrs.size() == 180 + 10 - 30 );

    auto stats = algo.stats();
    REQUIRE( stats.naiveOps == newPtrs.size() );
    REQUIRE( stats.diffOps == 10 + 30 + 2 );
    REQUIRE( algo.naive() == algo.diff() );
}

TEST_CASE("filter_algo_2") {
    auto [sets, base, basePtrs, newPtrs] = initVectors(10000, 200, 150, 40, 150);
    FilterAlgo algo(base, basePtrs, newPtrs);

    REQUIRE( basePtrs.size() == 150 );
    REQUIRE( newPtrs.size() == 150 + 40 - 150 );

    auto stats = algo.stats();
    REQUIRE( stats.naiveOps == newPtrs.size() );
    REQUIRE( stats.diffOps == 40 + 150 + 2 );
    REQUIRE( algo.naive() == algo.diff() );
}
