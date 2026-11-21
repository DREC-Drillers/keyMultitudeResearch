#pragma once

#include "backends/AVLTreeBackend.hpp"
#include "backends/RBTreeBackend.hpp"
#include "backends/bTreeBackend.hpp"
#include "backends/setBackend.hpp"
#include "backends/vectorBackend.hpp"

namespace key_multitude::benchmark_workload {

template <typename Callback>
void forEachBackend(Callback callback) {
    callback.template operator()<VectorBackend>("vector");
    callback.template operator()<SetBackend>("set");
    callback.template operator()<AVLTreeBackend>("avl");
    callback.template operator()<RBTreeBackend>("rb");
    callback.template operator()<BTreeBackend<16>>("btree16");
}

} // namespace key_multitude::benchmark_workload
