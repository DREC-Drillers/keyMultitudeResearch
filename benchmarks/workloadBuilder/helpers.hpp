#pragma once

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <numeric>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
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

template <typename Measure>
void registerInsertQueryRatioSweep(const char* workload, const char* backend,
                                   int size, int percentStep, Measure measure) {
    if (percentStep <= 0) {
        throw std::invalid_argument("Insert percentage step must be positive");
    }

    auto* entry = benchmark::RegisterBenchmark(std::string(workload) + "/" + backend,
                                               std::move(measure))
                      ->ArgNames({"size", "insert_pct"});
    for (int percent = 0; percent < 100; percent += percentStep) {
        entry->Args({size, percent});
    }
    entry->Args({size, 100});
}

inline std::vector<int> makePopulationKeys(int size) {
    std::vector<int> keys(static_cast<std::size_t>(size));
    std::iota(keys.begin(), keys.end(), 0);

    constexpr auto POPULATION_SHUFFLE_SEED = 42U;
    std::mt19937 random(POPULATION_SHUFFLE_SEED);
    std::shuffle(keys.begin(), keys.end(), random);

    for (int& key : keys) {
        key *= 2;
    }
    return keys;
}

template <KeyMultitudeBackend Backend>
void fillMultitude(KeyMultitude<Backend>& keys,
                   std::span<const int> populationKeys) {
    for (int key : populationKeys) {
        keys.insertKey(key);
    }
}

template <KeyMultitudeBackend Backend>
void fillMultitude(KeyMultitude<Backend>& keys, int size) {
    fillMultitude(keys, makePopulationKeys(size));
}

} // namespace key_multitude::benchmark_workload
