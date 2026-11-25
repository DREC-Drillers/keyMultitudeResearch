#pragma once

#include <cstddef>
#include <stdexcept>
#include <tree/avlTree.hpp>

namespace key_multitude {

class AVLTreeBackend {
public:
    bool insertKey(int key) { return tree_.insert(key); }

    std::size_t size() const noexcept { return tree_.size(); }

    std::size_t countKeysLessThan(int key) const { return tree_.rank(key); }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }
        return rankAfter(right) - rankAfter(left);
    }

    int getKeyBySortedIndex(std::size_t index) const { return tree_.select(index); }

private:
    std::size_t rankAfter(int key) const {
        const auto next = tree_.upper_bound(key);
        return next == tree_.end() ? tree_.size() : tree_.rank(*next);
    }

    general_functions::AVLTree<int> tree_;
};

} // namespace key_multitude
