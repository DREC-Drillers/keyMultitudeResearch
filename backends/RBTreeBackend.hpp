#pragma once

#include <cstddef>
#include <functional>

#include <tree/red_black_tree.hpp>

namespace key_multitude {

class RBTreeBackend {
public:
    bool insertKey(int key) {
        const auto previousSize = tree_.size();
        tree_.insert(key);
        return tree_.size() != previousSize;
    }

    std::size_t size() const noexcept { return tree_.size(); }
    std::size_t countKeysLessThan(int key) const;
    std::size_t countKeysInRange(int left, int right);
    int getKeyBySortedIndex(std::size_t index) const;
private:
    red_black_tree::redBlackTree_t<int, std::less<int>> tree_;
};

} // namespace key_multitude
