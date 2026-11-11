#pragma once

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace key_multitude {

template <class Backend>
concept KeyMultitudeBackend =
    requires(Backend& backend, const Backend& constBackend, int key, int left,
             int right, std::size_t zeroBasedIndex) {
        { backend.insertKey(key) } -> std::same_as<bool>;
        { constBackend.size() } -> std::same_as<std::size_t>;
        { constBackend.countKeysLessThan(key) } -> std::same_as<std::size_t>;
        { constBackend.countKeysInRange(left, right) } -> std::same_as<std::size_t>;
        { constBackend.getKeyBySortedIndex(zeroBasedIndex) } -> std::same_as<int>;
    };

template <KeyMultitudeBackend Backend>
class KeyMultitude {
public:
    using KeyType = int;
    using SizeType = std::size_t;

    KeyMultitude() = default;

    explicit KeyMultitude(Backend backend) : storage_(std::move(backend)) {}

    bool insertKey(KeyType key) { return storage_.insertKey(key); }

    SizeType size() const { return storage_.size(); }

    bool empty() const { return size() == 0; }

    SizeType countKeysLessThan(KeyType key) const {
        return storage_.countKeysLessThan(key);
    }

    SizeType countKeysInRange(KeyType left, KeyType right) const {
        if (left > right) {
            throw std::invalid_argument(
                "Left range boundary must not exceed right boundary");
        }

        return storage_.countKeysInRange(left, right);
    }

    KeyType findKthSmallestKey(SizeType oneBasedRank) const {
        if (oneBasedRank == 0 || oneBasedRank > size()) {
            throw std::out_of_range("KeyMultitude rank must be in [1, size()]");
        }

        return storage_.getKeyBySortedIndex(oneBasedRank - 1);
    }

private:
    Backend storage_{};
};

} // namespace key_multitude
