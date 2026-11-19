#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>

#include <tree/red_black_tree.hpp>

namespace key_multitude {

class RBTreeBackend {
private:
    red_black_tree::redBlackTree_t<int, std::less<int>> tree_;

public:
    bool insertKey(int key) {
        const auto previousSize = tree_.size();
        tree_.insert(key);
        return tree_.size() != previousSize;
    }

    std::size_t size() const noexcept { return tree_.size(); }

    std::size_t countKeysLessThan(int key) const { return tree_.countKeysLessThan(key); }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }
        return tree_.countKeysLessThanOrEqual(right) - tree_.countKeysLessThanOrEqual(left);
    }

    int getKeyBySortedIndex(std::size_t zeroBasedIndex) const {
        return tree_.select(zeroBasedIndex);
    }
};

} // namespace key_multitude
