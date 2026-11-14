#include "backends/bTreeBackend.hpp"
#include "backends/vectorBackend.hpp"
#include "keyMultitude.hpp"

template <typename Backend>
bool verifyBackend() {
    key_multitude::KeyMultitude<Backend> keys;
    keys.insertKey(42);
    keys.insertKey(42);
    keys.insertKey(-1);

    return keys.size() == 3 && keys.findKthSmallestKey(1) == -1 &&
           keys.findKthSmallestKey(3) == 42 && keys.countKeysLessThan(42) == 1 &&
           keys.countKeysInRange(-1, 42) == 2;
}

int main() {
    return verifyBackend<key_multitude::VectorBackend>() &&
                   verifyBackend<key_multitude::BTreeBackend<2>>()
               ? 0
               : 1;
}
