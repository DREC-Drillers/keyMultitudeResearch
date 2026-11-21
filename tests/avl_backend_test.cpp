#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "backends/AVLTreeBackend.hpp"
#include "keyMultitude.hpp"

namespace {
using AVLTree = key_multitude::AVLTreeBackend;

// Test-only validation. Production AVL has no checking or query code.
struct AVLInfo {
    int height;
    std::size_t size;
};

template <typename BinaryTree>
std::optional<AVLInfo>
validateNode(const BinaryTree& tree, typename BinaryTree::NodeId id,
             typename BinaryTree::NodeId parent, const int* lower, const int* upper,
             std::vector<bool>& visited) {
    if (id == BinaryTree::INVALID_NODE) {
        return AVLInfo{0, 0};
    }
    if (id >= tree.size() || visited[id]) {
        return std::nullopt;
    }
    visited[id] = true;
    const auto& link = tree.getNode(id);
    if (link.parent() != parent) {
        return std::nullopt;
    }
    const auto& node = link.data();
    if ((lower && node.key <= *lower) || (upper && node.key >= *upper)) {
        return std::nullopt;
    }
    const auto left = validateNode(tree, link.left(), id, lower, &node.key, visited);
    const auto right =
        validateNode(tree, link.right(), id, &node.key, upper, visited);
    if (!left || !right) {
        return std::nullopt;
    }
    const int height = 1 + std::max(left->height, right->height);
    const auto size = 1 + left->size + right->size;
    const int balance = left->height - right->height;
    if (balance < -1 || balance > 1 || node.height != height ||
        node.subtreeSize != size) {
        return std::nullopt;
    }
    return AVLInfo{height, size};
}

bool validateAVL(const AVLTree& backend) {
    const auto& tree = backend.tree();
    std::vector<bool> visited(tree.size(), false);
    const auto result =
        validateNode(tree,
                     tree.root(),
                     std::remove_cvref_t<decltype(tree)>::INVALID_NODE,
                     nullptr,
                     nullptr,
                     visited);
    return result && result->size == tree.size();
}

TEST(AVLBackendTest, EmptyAndInvalidQueries) {
    AVLTree tree;
    EXPECT_TRUE(validateAVL(tree));
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.countKeysLessThan(0), 0U);
    EXPECT_EQ(tree.countKeysInRange(-1, 1), 0U);
    EXPECT_THROW(tree.countKeysInRange(1, -1), std::invalid_argument);
    EXPECT_THROW(tree.getKeyBySortedIndex(0), std::out_of_range);
    tree.insertKey(3);
    EXPECT_THROW(tree.getKeyBySortedIndex(1), std::out_of_range);
    EXPECT_THROW(tree.getKeyBySortedIndex(std::numeric_limits<std::size_t>::max()),
                 std::out_of_range);
}

TEST(AVLBackendTest, AssignmentExampleAndUniqueKeys) {
    AVLTree tree;
    for (int key : {8, 2, -1}) {
        EXPECT_TRUE(tree.insertKey(key));
    }
    EXPECT_FALSE(tree.insertKey(2));
    EXPECT_EQ(tree.size(), 3U);
    EXPECT_EQ(tree.getKeyBySortedIndex(0), -1);
    EXPECT_EQ(tree.getKeyBySortedIndex(1), 2);
    EXPECT_EQ(tree.countKeysLessThan(3), 2U);
    EXPECT_EQ(tree.countKeysInRange(-1, 8), 2U);
    EXPECT_EQ(tree.countKeysInRange(2, 2), 0U);
    EXPECT_THROW(tree.getKeyBySortedIndex(tree.size()), std::out_of_range);
    EXPECT_THROW(tree.getKeyBySortedIndex(3), std::out_of_range);
}

TEST(AVLBackendTest, AllFourRotationCases) {
    for (const auto& input : {std::array{3, 2, 1},
                              std::array{1, 2, 3},
                              std::array{3, 1, 2},
                              std::array{1, 3, 2}}) {
        AVLTree tree;
        for (int key : input) {
            EXPECT_TRUE(tree.insertKey(key));
            ASSERT_TRUE(validateAVL(tree));
        }
        for (std::size_t index = 0; index < 3; ++index) {
            EXPECT_EQ(tree.getKeyBySortedIndex(index), static_cast<int>(index + 1));
            EXPECT_EQ(tree.countKeysLessThan(static_cast<int>(index + 1)), index);
        }
        const auto& binary = tree.tree();
        ASSERT_NE(binary.root(),
                  std::remove_cvref_t<decltype(binary)>::INVALID_NODE);
        const auto& root = binary.getNode(binary.root());
        ASSERT_NE(root.left(), std::remove_cvref_t<decltype(binary)>::INVALID_NODE);
        ASSERT_NE(root.right(), std::remove_cvref_t<decltype(binary)>::INVALID_NODE);
        EXPECT_EQ(root.data().key, 2);
        EXPECT_EQ(binary.getNode(root.left()).data().key, 1);
        EXPECT_EQ(binary.getNode(root.right()).data().key, 3);
    }
}

TEST(AVLBackendTest, ExtremeKeysAndRepeatedInsertions) {
    constexpr int MIN_KEY = std::numeric_limits<int>::min();
    constexpr int MAX_KEY = std::numeric_limits<int>::max();
    AVLTree tree;
    for (int key : {MIN_KEY, 0, MAX_KEY}) {
        EXPECT_TRUE(tree.insertKey(key));
    }
    for (int i = 0; i < 1000; ++i) {
        EXPECT_FALSE(tree.insertKey(0));
    }
    EXPECT_EQ(tree.size(), 3U);
    EXPECT_EQ(tree.countKeysInRange(MIN_KEY, MAX_KEY), 2U);
    EXPECT_EQ(tree.countKeysInRange(MIN_KEY, MIN_KEY), 0U);
    EXPECT_EQ(tree.countKeysLessThan(MIN_KEY), 0U);
    EXPECT_EQ(tree.countKeysLessThan(MAX_KEY), 2U);
    EXPECT_EQ(tree.getKeyBySortedIndex(0), MIN_KEY);
    EXPECT_EQ(tree.getKeyBySortedIndex(2), MAX_KEY);
    EXPECT_TRUE(validateAVL(tree));
}

TEST(AVLBackendTest, MixedOperationsMatchUniqueReference) {
    AVLTree tree;
    std::set<int> reference;
    std::mt19937 random(42);
    std::uniform_int_distribution<int> distribution(-1000, 1000);
    for (int step = 0; step < 2000; ++step) {
        const int key = distribution(random);
        EXPECT_EQ(tree.insertKey(key), reference.insert(key).second);
        ASSERT_TRUE(validateAVL(tree));
        ASSERT_EQ(tree.size(), reference.size());
        const int left = distribution(random);
        const int right = distribution(random);
        std::size_t expectedLess = 0;
        std::size_t expectedRange = 0;
        std::size_t index = 0;
        for (int stored : reference) {
            expectedLess += stored < key;
            expectedRange += left < stored && stored <= right;
            ASSERT_EQ(tree.getKeyBySortedIndex(index++), stored);
        }
        EXPECT_EQ(tree.countKeysLessThan(key), expectedLess);
        if (left <= right) {
            EXPECT_EQ(tree.countKeysInRange(left, right), expectedRange);
        } else {
            EXPECT_THROW(tree.countKeysInRange(left, right), std::invalid_argument);
        }
    }
}

TEST(AVLBackendTest, SortedReverseAndShuffledInputsStayBalanced) {
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), -5000);
    std::mt19937 random(1337);
    for (int order = 0; order < 3; ++order) {
        AVLTree tree;
        for (std::size_t i = 0; i < input.size(); ++i) {
            ASSERT_TRUE(tree.insertKey(input[i]));
            if (i % 100 == 0) {
                ASSERT_TRUE(validateAVL(tree));
            }
        }
        ASSERT_TRUE(validateAVL(tree));
        for (std::size_t i = 0; i < input.size(); ++i) {
            const int expected = static_cast<int>(i) - 5000;
            EXPECT_EQ(tree.getKeyBySortedIndex(i), expected);
            EXPECT_EQ(tree.countKeysLessThan(expected), i);
        }
        if (order == 0) {
            std::reverse(input.begin(), input.end());
        } else {
            std::shuffle(input.begin(), input.end(), random);
        }
    }
}

// This test intentionally checks that moved-from trees remain empty and reusable.
// NOLINTBEGIN(clang-analyzer-cplusplus.Move, bugprone-use-after-move)
TEST(AVLBackendTest, CopiesAreIndependentAndMovedFromTreesAreReusable) {
    AVLTree original;
    for (int key : {8, 2, -1, 5, 20}) {
        original.insertKey(key);
    }
    auto copy = original;
    copy.insertKey(9);
    original.insertKey(-5);
    EXPECT_EQ(copy.getKeyBySortedIndex(0), -1);
    EXPECT_EQ(original.getKeyBySortedIndex(0), -5);
    AVLTree assigned;
    assigned.insertKey(100);
    assigned = original;
    assigned.insertKey(7);
    EXPECT_EQ(original.size(), 6U);
    EXPECT_EQ(assigned.size(), 7U);
    auto& self = assigned;
    assigned = self;
    EXPECT_TRUE(validateAVL(assigned));
    AVLTree moved(std::move(copy));
    EXPECT_EQ(moved.size(), 6U);
    EXPECT_EQ(copy.size(), 0U);
    EXPECT_TRUE(copy.insertKey(42));
    assigned = std::move(original);
    EXPECT_EQ(assigned.size(), 6U);
    EXPECT_EQ(original.size(), 0U);
    EXPECT_TRUE(original.insertKey(99));
    assigned = std::move(self);
    for (const auto* tree : {&original, &assigned, &moved, &copy}) {
        EXPECT_TRUE(validateAVL(*tree));
    }
}
// NOLINTEND(clang-analyzer-cplusplus.Move, bugprone-use-after-move)
} // namespace

static_assert(key_multitude::KeyMultitudeBackend<key_multitude::AVLTreeBackend>);

TEST(AVLBackendTest, ConnectsLibraryTreeToCommonInterface) {
    key_multitude::KeyMultitude<key_multitude::AVLTreeBackend> keys;
    for (int key : {8, 2, -1}) {
        EXPECT_TRUE(keys.insertKey(key));
    }
    EXPECT_FALSE(keys.insertKey(2));
    EXPECT_EQ(keys.size(), 3U);
    EXPECT_EQ(keys.findKthSmallestKey(1), -1);
    EXPECT_EQ(keys.findKthSmallestKey(2), 2);
    EXPECT_EQ(keys.countKeysLessThan(3), 2U);
    EXPECT_EQ(keys.countKeysInRange(-1, 8), 2U);
    EXPECT_THROW(keys.countKeysInRange(8, -1), std::invalid_argument);
    EXPECT_THROW(keys.findKthSmallestKey(4), std::out_of_range);
}
