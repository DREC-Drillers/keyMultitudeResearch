#include <tree/avlTree.hpp>
#include <iostream>

int main() {
    general_functions::AVLTree tree;

    for (int key : {30, 20, 10, 25, 28, 40, 50, 20}) {
        std::cout << tree.insert(key) << "\n";
    }

}
