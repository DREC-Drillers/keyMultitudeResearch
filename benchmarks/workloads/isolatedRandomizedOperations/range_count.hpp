#pragma once

#include <algorithm>
#include <benchmark/benchmark.h>

#include "benchmarks/workloadBuilder/workloadBuilder.hpp"
#include "benchmarks/workloads/workloadTemplates.hpp"

namespace key_multitude::benchmark_workload {

struct RangeCountWorkload {
    static constexpr auto NAME = "range_count";

    template <KeyMultitudeBackend Backend>
    static void measure(benchmark::State& state) {
        const auto size = static_cast<int>(state.range(0));
        const int width = std::max(1, size / 100);
        const auto workload =
            WorkloadBuilder(size).sampleFrom(size - width + 1).build();

        measureReadQueries<Backend>(
            state, workload, [width](const auto& keys, int key) {
                const int left = key - 1;
                return keys.countKeysInRange(left, left + 2 * width);
            });
    }
};

} // namespace key_multitude::benchmark_workload
