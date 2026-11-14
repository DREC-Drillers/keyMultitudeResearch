#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace key_multitude {

class VectorBackend {
public:
    bool insertKey(int key) {
        const auto insertionIt = std::upper_bound(keys_.begin(), keys_.end(), key);
        keys_.insert(insertionIt, key);
        return true;
    }

    std::size_t size() const noexcept { return keys_.size(); }

    std::size_t countKeysLessThan(int key) const {
        return static_cast<std::size_t>(
            std::lower_bound(keys_.begin(), keys_.end(), key) - keys_.begin());
    }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }

        if (left == right) {
            return 0;
        }

        return static_cast<std::size_t>(
            std::upper_bound(keys_.begin(), keys_.end(), right) -
            std::upper_bound(keys_.begin(), keys_.end(), left));
    }

    int getKeyBySortedIndex(std::size_t zeroBasedIndex) const {
        return keys_.at(zeroBasedIndex);
    }

private:
    std::vector<int> keys_;
};

} // namespace key_multitude
