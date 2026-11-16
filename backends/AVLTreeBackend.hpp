#pragma once

#include <cstddef>
#include <stdexcept>
#include <tree/avlTree.hpp>

namespace key_multitude {

class AVLTreeBackend {
public:
    bool insertKey(int key) { return tree_.insert(key); }

    std::size_t size() const noexcept { return tree_.size(); }

    std::size_t countKeysLessThan(int key) const {
        return tree_.lower_bound(key);
    }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }
        return tree_.upper_bound(right) - tree_.upper_bound(left);
    }

    int getKeyBySortedIndex(std::size_t index) const { return tree_.select(index); }

    const auto& tree() const noexcept { return tree_.tree(); }

private:
    general_functions::AVLTree tree_;
};

} // namespace key_multitude
