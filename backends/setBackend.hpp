#pragma once

#include <cstddef>
#include <iterator>
#include <set>
#include <stdexcept>

namespace key_multitude {

class SetBackend {
public:
    bool insertKey(int key) { return keys_.insert(key).second; }

    std::size_t size() const noexcept { return keys_.size(); }

    std::size_t countKeysLessThan(int key) const {
        return static_cast<std::size_t>(
            std::distance(keys_.begin(), keys_.lower_bound(key)));
    }

    std::size_t countKeysInRange(int left, int right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }

        return static_cast<std::size_t>(
            std::distance(keys_.upper_bound(left), keys_.upper_bound(right)));
    }

    int getKeyBySortedIndex(std::size_t zeroBasedIndex) const {
        if (zeroBasedIndex >= keys_.size()) {
            throw std::out_of_range("SetBackend index must be in [0, size())");
        }

        return *std::next(keys_.begin(),
                          static_cast<std::ptrdiff_t>(zeroBasedIndex));
    }

private:
    std::set<int> keys_;
};

} // namespace key_multitude
