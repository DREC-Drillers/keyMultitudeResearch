#include <benchmark/benchmark.h>

#include "benchmarks/registerWorkload.hpp"

int main(int argc, char** argv) {
    key_multitude::benchmark_workload::registerWorkloads();

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
