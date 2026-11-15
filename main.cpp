#include <dump/treeDump.hpp>
#include <tree/avlTree.hpp>

int main() {
    general_functions::AVLTree tree;

    for (int key : {30, 20, 10, 25, 28, 40, 50}) {
        tree.insert(key);
    }

    //general_functions::dumpTree(tree.tree());
}
