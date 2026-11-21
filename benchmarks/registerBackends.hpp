#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "backends/AVLTreeBackend.hpp"
#include "backends/RBTreeBackend.hpp"
#include "backends/bTreeBackend.hpp"
#include "backends/setBackend.hpp"
#include "backends/vectorBackend.hpp"
#include "btreeDegrees.hpp"

namespace key_multitude::benchmark_workload {

template <typename Callback, std::size_t... Degrees>
void forEachBTreeBackend(Callback& callback, std::index_sequence<Degrees...>) {
    (callback.template operator()<BTreeBackend<Degrees>>(
         ("btree" + std::to_string(Degrees)).c_str()),
     ...);
}

template <typename Callback>
void forEachBackend(Callback callback) {
    callback.template operator()<VectorBackend>("vector");
    callback.template operator()<SetBackend>("set");
    callback.template operator()<AVLTreeBackend>("avl");
    callback.template operator()<RBTreeBackend>("rb");
    forEachBTreeBackend(callback, BenchmarkBTreeDegrees{});
}

} // namespace key_multitude::benchmark_workload
