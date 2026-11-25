#pragma once

#include <benchmark/benchmark.h>

#include "benchmarks/workloadBuilder/workloadBuilder.hpp"
#include "benchmarks/workloads/workloadTemplates.hpp"

namespace key_multitude::benchmark_workload {

struct CountLessWorkload {
    static constexpr auto NAME = "count_less";

    template <KeyMultitudeBackend Backend>
    static void measure(benchmark::State& state) {
        const auto workload =
            WorkloadBuilder(static_cast<int>(state.range(0))).build();
        measureReadQueries<Backend>(state, workload, [](const auto& keys, int key) {
            return keys.countKeysLessThan(key);
        });
    }
};

} // namespace key_multitude::benchmark_workload
