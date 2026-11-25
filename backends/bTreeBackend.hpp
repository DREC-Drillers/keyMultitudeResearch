#pragma once

#include <cstddef>
#include <stdexcept>
#include <tree/bTree.hpp>

namespace key_multitude {

template <std::size_t minDegree>
class BTreeBackend {
public:
    bool insertKey(int key) {
        tree_.insert(key);
        return true;
    }

    std::size_t size() const noexcept { return tree_.size(); }

    std::size_t countKeysLessThan(int key) const {
        return tree_.countKeysLessThan(key);
    }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }

        return tree_.countKeysLessThanOrEqual(right) -
               tree_.countKeysLessThanOrEqual(left);
    }

    int getKeyBySortedIndex(std::size_t zeroBasedIndex) const {
        return tree_.getKeyBySortedIndex(zeroBasedIndex);
    }

private:
    general_functions::BTree<int, minDegree> tree_;
};

} // namespace key_multitude
