#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <utility>

#include "benchmarks/workloadBuilder/helpers.hpp"
#include "benchmarks/workloadBuilder/workloadBuilder.hpp"
#include "keyMultitude.hpp"

namespace key_multitude::benchmark_workload {

template <KeyMultitudeBackend Backend, typename Query>
void measureReadQueries(benchmark::State& state, const PreparedWorkload& workload,
                        Query query) {
    KeyMultitude<Backend> keys;
    fillMultitude(keys, workload.multitudeStartSize);

    std::size_t offset = 0;
    for (auto iteration : state) {
        std::size_t checksum = 0;
        for (int i = 0; i < workload.keysPerBatch; ++i) {
            checksum += static_cast<std::size_t>(
                query(std::as_const(keys), workload.sampledKeys[offset++]));
        }
        benchmark::DoNotOptimize(checksum);
        if (offset == workload.sampledKeys.size()) {
            offset = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * workload.keysPerBatch);
}

} // namespace key_multitude::benchmark_workload
