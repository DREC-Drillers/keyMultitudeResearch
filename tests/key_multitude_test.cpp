#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "backends/bTreeBackend.hpp"
#include "backends/setBackend.hpp"
#include "backends/vectorBackend.hpp"
#include "keyMultitude.hpp"

namespace {

constexpr std::size_t LARGE_MIN_DEGREE = 32;

using Backends =
    ::testing::Types<key_multitude::VectorBackend, key_multitude::BTreeBackend<2>,
                     key_multitude::BTreeBackend<3>,
                     key_multitude::BTreeBackend<LARGE_MIN_DEGREE>>;

template <typename Backend>
class KeyMultitudeTest : public ::testing::Test {};

TYPED_TEST_SUITE(KeyMultitudeTest, Backends);

static_assert(key_multitude::KeyMultitudeBackend<key_multitude::VectorBackend>);
static_assert(key_multitude::KeyMultitudeBackend<key_multitude::SetBackend>);
static_assert(key_multitude::KeyMultitudeBackend<key_multitude::BTreeBackend<2>>);
static_assert(!key_multitude::KeyMultitudeBackend<int>);

TEST(SetBackendTest, StoresUniqueKeysAndSupportsOrderQueries) {
    key_multitude::KeyMultitude<key_multitude::SetBackend> keys;
    EXPECT_TRUE(keys.insertKey(8));
    EXPECT_TRUE(keys.insertKey(2));
    EXPECT_TRUE(keys.insertKey(-1));
    EXPECT_FALSE(keys.insertKey(2));

    EXPECT_EQ(keys.size(), 3U);
    EXPECT_EQ(keys.countKeysInRange(-1, 8), 2U);
    EXPECT_EQ(keys.countKeysInRange(2, 2), 0U);
    EXPECT_EQ(keys.countKeysLessThan(2), 1U);
    EXPECT_EQ(keys.countKeysLessThan(3), 2U);
    EXPECT_EQ(keys.findKthSmallestKey(1), -1);
    EXPECT_EQ(keys.findKthSmallestKey(2), 2);
    EXPECT_EQ(keys.findKthSmallestKey(3), 8);
    EXPECT_THROW(keys.findKthSmallestKey(0), std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(4), std::out_of_range);
    EXPECT_THROW(keys.countKeysInRange(8, -1), std::invalid_argument);
}

TYPED_TEST(KeyMultitudeTest, EmptyMultiset) {
    const key_multitude::KeyMultitude<TypeParam> keys;
    EXPECT_TRUE(keys.empty());
    EXPECT_EQ(keys.size(), 0U);
    EXPECT_EQ(keys.countKeysLessThan(10), 0U);
    EXPECT_EQ(keys.countKeysInRange(-10, 10), 0U);
    EXPECT_EQ(keys.countKeysInRange(0, 0), 0U);
    EXPECT_THROW(keys.countKeysInRange(1, 0), std::invalid_argument);
    EXPECT_THROW(keys.findKthSmallestKey(0), std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(1), std::out_of_range);
}

TYPED_TEST(KeyMultitudeTest, StoresDuplicateKeysInSortedOrder) {
    key_multitude::KeyMultitude<TypeParam> keys;
    for (int key : {8, 2, -1, 2}) {
        EXPECT_TRUE(keys.insertKey(key));
    }

    EXPECT_EQ(keys.size(), 4U);
    EXPECT_FALSE(keys.empty());
    EXPECT_EQ(keys.findKthSmallestKey(1), -1);
    EXPECT_EQ(keys.findKthSmallestKey(2), 2);
    EXPECT_EQ(keys.findKthSmallestKey(3), 2);
    EXPECT_EQ(keys.findKthSmallestKey(4), 8);
    EXPECT_THROW(keys.findKthSmallestKey(5), std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(std::numeric_limits<std::size_t>::max()),
                 std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(0), std::out_of_range);
}

TYPED_TEST(KeyMultitudeTest, QueryBoundariesIncludeEveryOccurrence) {
    key_multitude::KeyMultitude<TypeParam> keys;
    for (int key : {-1, 2, 8, -1, 2, 8}) {
        keys.insertKey(key);
    }

    EXPECT_EQ(keys.countKeysLessThan(-1), 0U);
    EXPECT_EQ(keys.countKeysLessThan(2), 2U);
    EXPECT_EQ(keys.countKeysLessThan(3), 4U);
    EXPECT_EQ(keys.countKeysInRange(-1, 8), 4U);
    EXPECT_EQ(keys.countKeysInRange(-2, 8), 6U);
    EXPECT_EQ(keys.countKeysInRange(2, 2), 0U);
    EXPECT_THROW(keys.countKeysInRange(8, -1), std::invalid_argument);
}

TYPED_TEST(KeyMultitudeTest, ExtremeIntegerKeys) {
    constexpr int MIN_KEY = std::numeric_limits<int>::min();
    constexpr int MAX_KEY = std::numeric_limits<int>::max();
    key_multitude::KeyMultitude<TypeParam> keys;
    for (int key : {MIN_KEY, 0, MAX_KEY, MIN_KEY, 0, MAX_KEY}) {
        keys.insertKey(key);
    }

    EXPECT_EQ(keys.countKeysInRange(MIN_KEY, MAX_KEY), 4U);
    EXPECT_EQ(keys.countKeysLessThan(MIN_KEY), 0U);
    EXPECT_EQ(keys.countKeysLessThan(MAX_KEY), 4U);
    EXPECT_EQ(keys.findKthSmallestKey(1), MIN_KEY);
    EXPECT_EQ(keys.findKthSmallestKey(6), MAX_KEY);
    EXPECT_EQ(keys.countKeysInRange(MAX_KEY, MAX_KEY), 0U);
}

TYPED_TEST(KeyMultitudeTest, AcceptsPopulatedBackend) {
    TypeParam backend;
    backend.insertKey(9);
    backend.insertKey(1);
    backend.insertKey(9);
    const key_multitude::KeyMultitude<TypeParam> keys(std::move(backend));

    EXPECT_EQ(keys.size(), 3U);
    EXPECT_EQ(keys.findKthSmallestKey(1), 1);
    EXPECT_EQ(keys.findKthSmallestKey(2), 9);
    EXPECT_EQ(keys.findKthSmallestKey(3), 9);
}

TYPED_TEST(KeyMultitudeTest, BackendValidatesRangesAndIndicesDirectly) {
    TypeParam backend;
    EXPECT_THROW(backend.getKeyBySortedIndex(0), std::out_of_range);
    backend.insertKey(3);

    EXPECT_EQ(backend.countKeysInRange(3, 3), 0U);
    EXPECT_THROW(backend.countKeysInRange(4, 2), std::invalid_argument);
    EXPECT_EQ(backend.getKeyBySortedIndex(0), 3);
    EXPECT_THROW(backend.getKeyBySortedIndex(1), std::out_of_range);
    EXPECT_THROW(
        backend.getKeyBySortedIndex(std::numeric_limits<std::size_t>::max()),
        std::out_of_range);
}

TYPED_TEST(KeyMultitudeTest, CopyHasIndependentStorage) {
    key_multitude::KeyMultitude<TypeParam> original;
    for (int key : {3, 1, 3, 2}) {
        original.insertKey(key);
    }

    auto copy = original;
    copy.insertKey(3);
    original.insertKey(-1);

    EXPECT_EQ(original.size(), 5U);
    EXPECT_EQ(copy.size(), 5U);
    EXPECT_EQ(original.countKeysInRange(2, 3), 2U);
    EXPECT_EQ(copy.countKeysInRange(2, 3), 3U);
    EXPECT_EQ(original.findKthSmallestKey(1), -1);
    EXPECT_EQ(copy.findKthSmallestKey(1), 1);
}

TYPED_TEST(KeyMultitudeTest, MixedOperationsMatchReferenceMultiset) {
    key_multitude::KeyMultitude<TypeParam> keys;
    std::multiset<int> reference;
    std::mt19937 random(42);
    std::uniform_int_distribution<int> distribution(-100, 100);

    for (int step = 0; step < 500; ++step) {
        const int key = distribution(random);
        EXPECT_TRUE(keys.insertKey(key));
        reference.insert(key);
        ASSERT_EQ(keys.size(), reference.size());
        const int left = distribution(random);
        const int right = distribution(random);
        std::size_t expectedLess = 0;
        std::size_t expectedRange = 0;
        std::size_t rank = 1;

        for (int stored : reference) {
            expectedLess += stored < key;
            expectedRange += left < stored && stored <= right;
            ASSERT_EQ(keys.findKthSmallestKey(rank++), stored);
        }

        EXPECT_EQ(keys.countKeysLessThan(key), expectedLess);

        if (left > right) {
            EXPECT_THROW(keys.countKeysInRange(left, right), std::invalid_argument);
        } else {
            EXPECT_EQ(keys.countKeysInRange(left, right), expectedRange);
        }
    }
}

TYPED_TEST(KeyMultitudeTest, SortedReverseAndShuffledSequences) {
    std::vector<int> input(2500);
    std::iota(input.begin(), input.end(), -1250);
    std::mt19937 random(1337);

    for (int order = 0; order < 3; ++order) {
        key_multitude::KeyMultitude<TypeParam> keys;

        for (int key : input) {
            keys.insertKey(key);
        }

        auto expected = input;
        std::sort(expected.begin(), expected.end());

        for (std::size_t index = 0; index < expected.size(); ++index) {
            ASSERT_EQ(keys.findKthSmallestKey(index + 1), expected[index]);
            ASSERT_EQ(keys.countKeysLessThan(expected[index]), index);
        }

        if (order == 0) {
            std::reverse(input.begin(), input.end());
        } else {
            std::shuffle(input.begin(), input.end(), random);
        }
    }
}

TYPED_TEST(KeyMultitudeTest, IdenticalKeysAcrossManySplits) {
    key_multitude::KeyMultitude<TypeParam> keys;
    constexpr std::size_t KEY_COUNT = 1000;

    for (std::size_t index = 0; index < KEY_COUNT; ++index) {
        keys.insertKey(7);
    }

    EXPECT_EQ(keys.size(), KEY_COUNT);
    EXPECT_EQ(keys.countKeysLessThan(7), 0U);
    EXPECT_EQ(keys.countKeysLessThan(8), KEY_COUNT);
    EXPECT_EQ(keys.countKeysInRange(6, 7), KEY_COUNT);
    EXPECT_EQ(keys.countKeysInRange(7, 8), 0U);

    for (std::size_t rank = 1; rank <= KEY_COUNT; ++rank) {
        ASSERT_EQ(keys.findKthSmallestKey(rank), 7);
    }
}

} // namespace
