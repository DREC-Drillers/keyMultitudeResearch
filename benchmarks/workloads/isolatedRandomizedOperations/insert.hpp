#pragma once

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>

#include "benchmarks/workloadBuilder/helpers.hpp"
#include "benchmarks/workloadBuilder/workloadBuilder.hpp"

namespace key_multitude::benchmark_workload {

struct InsertWorkload {
    static constexpr auto NAME = "insert";

    template <KeyMultitudeBackend Backend>
    static void measure(benchmark::State& state) {
        const auto size = static_cast<int>(state.range(0));
        const int keysPerBatch = std::clamp(size / 100, 16, 256);
        const auto workload =
            WorkloadBuilder(size).singleBatch(keysPerBatch).build();
        const auto populationKeys = makePopulationKeys(workload.multitudeStartSize);

        for ([[maybe_unused]] auto iteration : state) {
            state.PauseTiming();
            {
                KeyMultitude<Backend> keys;
                fillMultitude(keys, populationKeys);

                state.ResumeTiming();
                std::size_t inserted = 0;
                for (int key : workload.sampledKeys) {
                    inserted += keys.insertKey(key + 1);
                }
                benchmark::DoNotOptimize(inserted);
                state.PauseTiming();
            }
            state.ResumeTiming();
        }
        state.SetItemsProcessed(state.iterations() * workload.keysPerBatch);
    }
};

} // namespace key_multitude::benchmark_workload
