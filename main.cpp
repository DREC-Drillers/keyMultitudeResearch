#include <cstdlib>
#include <exception>
#include <iostream>
#include <type_traits>
#include <variant>

#include "backends/AVLTreeBackend.hpp"
#include "keyMultitude.hpp"
#include "parser/parser.hpp"

int main(int argc, char* argv[]) {
    using namespace key_multitude;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input-file>\n";
        return EXIT_FAILURE;
    }

    try {
        const auto commands = Parser{}.parse(argv[1]);
        KeyMultitude<AVLTreeBackend> keys;

        for (const auto& command : commands) {
            if (const auto* op = std::get_if<InsertKey>(&command)) {
                keys.insertKey(op->key);
            } else if (const auto* op = std::get_if<CountInRange>(&command)) {
                std::cout << keys.countKeysInRange(op->left, op->right) << ' ';
            } else if (const auto* op = std::get_if<FindKthSmallest>(&command)) {
                std::cout << keys.findKthSmallestKey(op->rank) << ' ';
            } else if (const auto* op = std::get_if<CountLessThan>(&command)) {
                std::cout << keys.countKeysLessThan(op->key) << ' ';
            }
        }

        std::cout << '\n';

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
