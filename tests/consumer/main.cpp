#include "backends/vectorBackend.hpp"
#include "keyMultitude.hpp"

int main() {
    key_multitude::KeyMultitude<key_multitude::VectorBackend> keys;
    keys.insertKey(42);
    return keys.size() == 1 && keys.findKthSmallestKey(1) == 42 ? 0 : 1;
}
