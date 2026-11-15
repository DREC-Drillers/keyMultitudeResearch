#pragma once

#include <cstddef>
#include <stdexcept>
#include <tree/avlTree.hpp>

namespace key_multitude {

// Express the research queries through the library search-tree API.
class AVLTreeBackend {
public:
    bool insertKey(int key) { return tree_.insert(key); }

    std::size_t size() const noexcept { return tree_.size(); }

    std::size_t countKeysLessThan(int key) const {
        return static_cast<std::size_t>(
            tree_.distance(tree_.begin(), tree_.lower_bound(key)));
    }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }
        return static_cast<std::size_t>(
            tree_.distance(tree_.upper_bound(left), tree_.upper_bound(right)));
    }

    int getKeyBySortedIndex(std::size_t index) const { return tree_.select(index); }

    const auto& tree() const noexcept { return tree_.tree(); }

private:
    general_functions::AVLTree tree_;
};

} // namespace key_multitude
