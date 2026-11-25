#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <variant>

#include <gtest/gtest.h>

#include "backends/AVLTreeBackend.hpp"
#include "backends/RBTreeBackend.hpp"
#include "backends/bTreeBackend.hpp"
#include "backends/setBackend.hpp"
#include "backends/vectorBackend.hpp"
#include "tests/range_test_workload.hpp"

namespace {

using key_multitude::KeyMultitude;
using key_multitude::test_workload::Insert;
using key_multitude::test_workload::Operation;
using key_multitude::test_workload::RangeQuery;

template <typename Backend>
class RangeWorkloadTest : public ::testing::Test {};

using Backends =
    ::testing::Types<key_multitude::VectorBackend, key_multitude::SetBackend,
                     key_multitude::AVLTreeBackend, key_multitude::RBTreeBackend,
                     key_multitude::BTreeBackend<16>>;
TYPED_TEST_SUITE(RangeWorkloadTest, Backends);

TYPED_TEST(RangeWorkloadTest, MatchesReferenceForEveryOperation) {
    constexpr std::uint32_t SEED = 42;
    constexpr int INITIAL_SIZE = 64;
    constexpr int OPERATION_COUNT = 128;
    for (const int insertCount : {0, OPERATION_COUNT / 2}) {
        SCOPED_TRACE(::testing::Message()
                     << "seed=" << SEED << ", inserts=" << insertCount);
        const auto workload = key_multitude::test_workload::makeRangeWorkload(
            INITIAL_SIZE, insertCount, OPERATION_COUNT - insertCount, SEED);
        KeyMultitude<TypeParam> keys;
        std::set<int> reference;
        for (int key : workload.initialKeys) {
            ASSERT_TRUE(reference.insert(key).second);
            ASSERT_TRUE(keys.insertKey(key));
        }

        std::size_t operationIndex = 0;
        key_multitude::test_workload::replay(
            keys, workload, [&](const Operation& operation, std::size_t result) {
                SCOPED_TRACE(::testing::Message()
                             << "operation=" << operationIndex++);
                if (const auto* insert = std::get_if<Insert>(&operation)) {
                    SCOPED_TRACE(::testing::Message() << "insert " << insert->key);
                    EXPECT_TRUE(reference.insert(insert->key).second);
                    EXPECT_EQ(result, 1U);
                } else {
                    const auto& query = std::get<RangeQuery>(operation);
                    SCOPED_TRACE(::testing::Message() << "range (" << query.left
                                                      << ", " << query.right << "]");
                    const auto first = reference.upper_bound(query.left);
                    const auto last = reference.upper_bound(query.right);
                    EXPECT_EQ(result,
                              static_cast<std::size_t>(std::distance(first, last)));
                }
            });

        EXPECT_EQ(operationIndex, OPERATION_COUNT);
        EXPECT_EQ(reference.size(),
                  static_cast<std::size_t>(INITIAL_SIZE + insertCount));
        EXPECT_EQ(keys.size(), reference.size());
    }
}

} // namespace
