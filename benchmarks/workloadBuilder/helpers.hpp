#pragma once

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <numeric>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "keyMultitude.hpp"

namespace key_multitude::benchmark_workload {

inline void registerMultitudeSizeSweep(const char* workload, const char* backend,
                                       void (*measure)(benchmark::State&),
                                       std::span<const int> sizes) {
    auto* entry =
        benchmark::RegisterBenchmark(std::string(workload) + "/" + backend, measure)
            ->ArgName("size");
    for (int size : sizes) {
        entry->Arg(size);
    }
}

template <KeyMultitudeBackend Backend>
void fillMultitude(KeyMultitude<Backend>& keys, int size) {
    std::vector<int> positions(static_cast<std::size_t>(size));
    std::iota(positions.begin(), positions.end(), 0);

    constexpr auto POPULATION_SHUFFLE_SEED = 42U;
    std::mt19937 random(POPULATION_SHUFFLE_SEED);
    std::shuffle(positions.begin(), positions.end(), random);

    for (int position : positions) {
        keys.insertKey(2 * position);
    }
}

} // namespace key_multitude::benchmark_workload
