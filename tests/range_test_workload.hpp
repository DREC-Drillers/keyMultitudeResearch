#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <variant>
#include <vector>

#include "keyMultitude.hpp"

namespace key_multitude::test_workload {

struct Insert {
    int key;
};

struct RangeQuery {
    // Count keys in (left, right].
    int left;
    int right;
};

using Operation = std::variant<Insert, RangeQuery>;

struct RangeWorkload {
    std::vector<int> initialKeys;
    std::vector<Operation> operations;
};

inline RangeWorkload makeRangeWorkload(int initialSize, int insertCount,
                                       int queryCount, std::uint32_t seed) {
    std::mt19937 random(seed);
    const int domainSize = 4 * (initialSize + insertCount + queryCount);
    std::vector<int> keyPool(static_cast<std::size_t>(domainSize));
    std::iota(keyPool.begin(), keyPool.end(), 0);
    std::shuffle(keyPool.begin(), keyPool.end(), random);

    RangeWorkload workload;
    workload.initialKeys.assign(keyPool.begin(), keyPool.begin() + initialSize);
    workload.operations.reserve(static_cast<std::size_t>(insertCount + queryCount));

    for (int i = 0; i < insertCount; ++i) {
        workload.operations.emplace_back(
            Insert{keyPool[static_cast<std::size_t>(initialSize + i)]});
    }

    const int rangeWidth = std::max(1, domainSize / 16);
    std::uniform_int_distribution<int> leftBoundary(-1, domainSize - 1 - rangeWidth);
    for (int i = 0; i < queryCount; ++i) {
        const int left = leftBoundary(random);
        workload.operations.emplace_back(RangeQuery{left, left + rangeWidth});
    }

    std::shuffle(workload.operations.begin(), workload.operations.end(), random);
    return workload;
}

template <KeyMultitudeBackend Backend, typename Observe>
void replay(KeyMultitude<Backend>& keys, const RangeWorkload& workload,
            Observe observe) {
    for (const auto& operation : workload.operations) {
        if (const auto* insert = std::get_if<Insert>(&operation)) {
            observe(operation,
                    static_cast<std::size_t>(keys.insertKey(insert->key)));
        } else {
            const auto& query = std::get<RangeQuery>(operation);
            observe(operation, keys.countKeysInRange(query.left, query.right));
        }
    }
}

} // namespace key_multitude::test_workload
