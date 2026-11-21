#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>

#include "benchmarks/workloadBuilder/workloadBuilder.hpp"
#include "benchmarks/workloads/workloadTemplates.hpp"

namespace key_multitude::benchmark_workload {

struct SelectRankWorkload {
    static constexpr auto NAME = "select_rank";

    template <KeyMultitudeBackend Backend>
    static void measure(benchmark::State& state) {
        const auto workload =
            WorkloadBuilder(static_cast<int>(state.range(0))).build();

        measureReadQueries<Backend>(state, workload, [](const auto& keys, int key) {
            return keys.findKthSmallestKey(static_cast<std::size_t>(key / 2) + 1);
        });
    }
};

} // namespace key_multitude::benchmark_workload
