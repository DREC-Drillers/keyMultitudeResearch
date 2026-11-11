#include <cstddef>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include "backends/vectorBackend.hpp"
#include "keyMultitude.hpp"

namespace {

using VectorMultitude = key_multitude::KeyMultitude<key_multitude::VectorBackend>;

static_assert(key_multitude::KeyMultitudeBackend<key_multitude::VectorBackend>);
static_assert(!key_multitude::KeyMultitudeBackend<int>);

TEST(KeyMultitudeTest, EmptySet) {
    const VectorMultitude keys;
    EXPECT_TRUE(keys.empty());
    EXPECT_EQ(keys.size(), 0U);
    EXPECT_EQ(keys.countKeysLessThan(10), 0U);
    EXPECT_EQ(keys.countKeysInRange(-10, 10), 0U);
    EXPECT_EQ(keys.countKeysInRange(0, 0), 0U);
    EXPECT_THROW(keys.countKeysInRange(1, 0), std::invalid_argument);
    EXPECT_THROW(keys.findKthSmallestKey(0), std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(1), std::out_of_range);
}

TEST(KeyMultitudeTest, InsertUniqueKeysInOrder) {
    VectorMultitude keys;
    EXPECT_TRUE(keys.insertKey(8));
    EXPECT_TRUE(keys.insertKey(2));
    EXPECT_TRUE(keys.insertKey(-1));
    EXPECT_FALSE(keys.insertKey(2));
    EXPECT_EQ(keys.size(), 3U);
    EXPECT_FALSE(keys.empty());
    EXPECT_EQ(keys.findKthSmallestKey(1), -1);
    EXPECT_EQ(keys.findKthSmallestKey(2), 2);
    EXPECT_EQ(keys.findKthSmallestKey(3), 8);
    EXPECT_THROW(keys.findKthSmallestKey(4), std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(std::numeric_limits<std::size_t>::max()),
                 std::out_of_range);
    EXPECT_THROW(keys.findKthSmallestKey(0), std::out_of_range);
}

TEST(KeyMultitudeTest, QueryBoundaries) {
    VectorMultitude keys;
    for (int key : {-1, 2, 8})
        keys.insertKey(key);
    EXPECT_EQ(keys.countKeysLessThan(-1), 0U);
    EXPECT_EQ(keys.countKeysLessThan(2), 1U);
    EXPECT_EQ(keys.countKeysLessThan(3), 2U);
    EXPECT_EQ(keys.countKeysInRange(-1, 8), 2U);
    EXPECT_EQ(keys.countKeysInRange(-2, 8), 3U);
    EXPECT_EQ(keys.countKeysInRange(2, 2), 0U);
    EXPECT_THROW(keys.countKeysInRange(8, -1), std::invalid_argument);
}

TEST(KeyMultitudeTest, ExtremeIntegerKeys) {
    constexpr int MIN_KEY = std::numeric_limits<int>::min();
    constexpr int MAX_KEY = std::numeric_limits<int>::max();
    VectorMultitude keys;
    keys.insertKey(MIN_KEY);
    keys.insertKey(0);
    keys.insertKey(MAX_KEY);
    EXPECT_EQ(keys.countKeysInRange(MIN_KEY, MAX_KEY), 2U);
    EXPECT_EQ(keys.countKeysLessThan(MIN_KEY), 0U);
    EXPECT_EQ(keys.countKeysLessThan(MAX_KEY), 2U);
    EXPECT_EQ(keys.findKthSmallestKey(1), MIN_KEY);
    EXPECT_EQ(keys.findKthSmallestKey(3), MAX_KEY);
}

TEST(KeyMultitudeTest, AcceptsPopulatedBackend) {
    key_multitude::VectorBackend backend;
    backend.insertKey(9);
    backend.insertKey(1);
    const VectorMultitude keys(std::move(backend));
    EXPECT_EQ(keys.size(), 2U);
    EXPECT_EQ(keys.findKthSmallestKey(1), 1);
    EXPECT_EQ(keys.findKthSmallestKey(2), 9);
}

TEST(VectorBackendTest, HandlesEmptyAndReversedRangesDirectly) {
    key_multitude::VectorBackend backend;
    backend.insertKey(3);
    EXPECT_EQ(backend.countKeysInRange(3, 3), 0U);
    EXPECT_THROW(backend.countKeysInRange(4, 2), std::invalid_argument);
    EXPECT_EQ(backend.getKeyBySortedIndex(0), 3);
}

TEST(KeyMultitudeTest, MixedOperationsMatchReferenceSet) {
    VectorMultitude keys;
    std::set<int> reference;
    std::mt19937 random(42);
    std::uniform_int_distribution<int> distribution(-100, 100);
    for (int step = 0; step < 250; ++step) {
        const int key = distribution(random);
        EXPECT_EQ(keys.insertKey(key), reference.insert(key).second);
        ASSERT_EQ(keys.size(), reference.size());
        const int left = distribution(random);
        const int right = distribution(random);
        std::size_t expectedLess = 0;
        std::size_t expectedRange = 0;
        std::size_t index = 1;
        for (int stored : reference) {
            expectedLess += stored < key;
            expectedRange += left < stored && stored <= right;
            EXPECT_EQ(keys.findKthSmallestKey(index++), stored);
        }
        EXPECT_EQ(keys.countKeysLessThan(key), expectedLess);
        if (left > right) {
            EXPECT_THROW(keys.countKeysInRange(left, right), std::invalid_argument);
        } else {
            EXPECT_EQ(keys.countKeysInRange(left, right), expectedRange);
        }
    }
}

} // namespace
