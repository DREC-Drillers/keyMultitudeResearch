#pragma once

#include <algorithm>
#include <array>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <random>
#include <span>
#include <utility>
#include <vector>

#include "benchmarks/workloadBuilder/workloadBuilder.hpp"
#include "benchmarks/workloads/workloadTemplates.hpp"

namespace key_multitude::benchmark_workload {

enum class MixedOperation { Insert, RangeCount, SelectRank, CountLess };

struct MixedRequest {
    MixedOperation operation;
    int argument;
};

inline constexpr int MIXED_BATCH_SIZE = 300;

inline std::vector<MixedRequest>
makeMixedRequests(int size, int insertPercent,
                  std::span<const MixedOperation> queries) {
    constexpr unsigned REQUEST_SEED = 42;
    const int insertCount = MIXED_BATCH_SIZE * insertPercent / 100;
    std::vector<MixedRequest> requests;
    requests.reserve(MIXED_BATCH_SIZE);

    if (insertCount > 0) {
        const auto inserts = WorkloadBuilder(size).singleBatch(insertCount).build();
        for (const int key : inserts.sampledKeys) {
            requests.push_back({MixedOperation::Insert, key + 1});
        }
    }

    for (int i = 0; i < MIXED_BATCH_SIZE - insertCount; ++i) {
        requests.push_back(
            {queries[static_cast<std::size_t>(i) % queries.size()], 0});
    }

    std::mt19937 random(REQUEST_SEED);
    std::shuffle(requests.begin(), requests.end(), random);

    int currentSize = size;
    const int rangeWidth = std::max(1, size / 100);
    for (auto& request : requests) {
        switch (request.operation) {
            case MixedOperation::Insert:
                ++currentSize;
                break;
            case MixedOperation::RangeCount:
                request.argument = 2 * std::uniform_int_distribution<int>(
                                           0, size - rangeWidth)(random) -
                                   1;
                break;
            case MixedOperation::SelectRank:
                request.argument =
                    std::uniform_int_distribution<int>(1, currentSize)(random);
                break;
            case MixedOperation::CountLess:
                request.argument =
                    std::uniform_int_distribution<int>(0, 2 * size)(random);
                break;
        }
    }
    return requests;
}

struct InsertQueryRatioWorkload {
private:
    static constexpr std::array QUERY_OPERATIONS{
        MixedOperation::RangeCount,
        MixedOperation::SelectRank,
        MixedOperation::CountLess,
    };
    static constexpr std::span<const MixedOperation> QUERIES{QUERY_OPERATIONS};

public:
    static constexpr std::array PROFILES{
        std::pair{"mixed_range_count", QUERIES.subspan(0, 1)},
        std::pair{"mixed_select_rank", QUERIES.subspan(1, 1)},
        std::pair{"mixed_count_less", QUERIES.subspan(2, 1)},
        std::pair{"mixed_all_queries", QUERIES},
    };

    template <KeyMultitudeBackend Backend>
    static void measure(benchmark::State& state,
                        std::span<const MixedOperation> queries) {
        const auto size = static_cast<int>(state.range(0));
        const auto percent = static_cast<int>(state.range(1));
        const int rangeWidth = std::max(1, size / 100);
        const auto requests = makeMixedRequests(size, percent, queries);

        measureWithFreshMultitude<Backend>(
            state, size, MIXED_BATCH_SIZE, [&requests, rangeWidth](auto& keys) {
                std::size_t checksum = 0;
                for (const auto& request : requests) {
                    switch (request.operation) {
                        case MixedOperation::Insert:
                            checksum += keys.insertKey(request.argument);
                            break;
                        case MixedOperation::RangeCount:
                            checksum += keys.countKeysInRange(
                                request.argument, request.argument + 2 * rangeWidth);
                            break;
                        case MixedOperation::SelectRank:
                            checksum +=
                                static_cast<std::size_t>(keys.findKthSmallestKey(
                                    static_cast<std::size_t>(request.argument)));
                            break;
                        case MixedOperation::CountLess:
                            checksum += keys.countKeysLessThan(request.argument);
                            break;
                    }
                }
                return checksum;
            });
    }
};

} // namespace key_multitude::benchmark_workload
