#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace key_multitude::benchmark_workload {

struct PreparedWorkload {
    int multitudeStartSize;
    int keysPerBatch;
    std::vector<int> sampledKeys;
};

class WorkloadBuilder {
public:
    explicit WorkloadBuilder(int multitudeStartSize)
        : multitudeStartSize_(multitudeStartSize),
          positionCount_(multitudeStartSize) {}

    WorkloadBuilder& sampleFrom(int positionCount) {
        positionCount_ = positionCount;
        return *this;
    }

    WorkloadBuilder& singleBatch(int keysPerBatch) {
        keysPerBatch_ = keysPerBatch;
        batchCount_ = 1;
        return *this;
    }

    PreparedWorkload build() const {
        if (multitudeStartSize_ <= 0 || keysPerBatch_ <= 0 ||
            positionCount_ < keysPerBatch_ ||
            positionCount_ > std::numeric_limits<int>::max() / KEY_STRIDE + 1) {
            throw std::invalid_argument(
                "Workload requires distinct keys in the supported integer range");
        }

        PreparedWorkload workload{multitudeStartSize_, keysPerBatch_, {}};
        workload.sampledKeys.reserve(static_cast<std::size_t>(keysPerBatch_) *
                                     batchCount_);
        std::mt19937 random(SAMPLE_SEED);
        for (int batch = 0; batch < batchCount_; ++batch) {
            appendSampleBlock(workload.sampledKeys, random);
        }
        return workload;
    }

private:
    static constexpr std::uint32_t SAMPLE_SEED = 42;
    static constexpr int DEFAULT_KEYS_PER_BATCH = 32;
    static constexpr int DEFAULT_BATCH_COUNT = 128;
    static constexpr int KEY_STRIDE = 2;

    void appendSampleBlock(std::vector<int>& keys, std::mt19937& random) const {
        const auto start = keys.size();
        for (int i = 0; i < keysPerBatch_; ++i) {
            const auto lower = static_cast<int>(static_cast<std::int64_t>(i) *
                                                positionCount_ / keysPerBatch_);
            const auto upper = static_cast<int>(static_cast<std::int64_t>(i + 1) *
                                                    positionCount_ / keysPerBatch_ -
                                                1);
            keys.push_back(KEY_STRIDE *
                           std::uniform_int_distribution<int>(lower, upper)(random));
        }
        using Difference = std::vector<int>::difference_type;
        std::shuffle(
            keys.begin() + static_cast<Difference>(start), keys.end(), random);
    }

    int multitudeStartSize_;
    int positionCount_;
    int keysPerBatch_ = DEFAULT_KEYS_PER_BATCH;
    int batchCount_ = DEFAULT_BATCH_COUNT;
};

} // namespace key_multitude::benchmark_workload
