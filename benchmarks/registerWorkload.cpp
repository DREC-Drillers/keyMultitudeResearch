#include "benchmarks/registerWorkload.hpp"

#include <span>

#include "benchmarks/registerBackends.hpp"
#include "benchmarks/workloadBuilder/helpers.hpp"
#include "benchmarks/workloads/isolatedRandomizedOperations/count_less.hpp"
#include "benchmarks/workloads/isolatedRandomizedOperations/insert.hpp"
#include "benchmarks/workloads/isolatedRandomizedOperations/range_count.hpp"
#include "benchmarks/workloads/isolatedRandomizedOperations/select_rank.hpp"

namespace key_multitude::benchmark_workload {

namespace {

constexpr int DEFAULT_MULTITUDE_SIZES[] = {
    200,
    1'000,
    4'000,
    16'000,
    64'000,
    250'000,
    1'000'000,
};

template <typename Workload>
void registerWorkload(std::span<const int> sizes) {
    forEachBackend([sizes]<typename Backend>(const char* backend) {
        registerMultitudeSizeSweep(
            Workload::NAME, backend, Workload::template measure<Backend>, sizes);
    });
}

} // namespace

void registerWorkloads() {
    registerWorkload<InsertWorkload>(DEFAULT_MULTITUDE_SIZES);
    registerWorkload<RangeCountWorkload>(DEFAULT_MULTITUDE_SIZES);
    registerWorkload<SelectRankWorkload>(DEFAULT_MULTITUDE_SIZES);
    registerWorkload<CountLessWorkload>(DEFAULT_MULTITUDE_SIZES);
}

} // namespace key_multitude::benchmark_workload
