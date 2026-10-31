#include "cookptrrbtree.h"

int main()
{
	RedBlackTree rbTree;

	rbTree.insert(24);
	rbTree.insert(33);
	rbTree.insert(42);
	rbTree.insert(51);
	rbTree.insert(60);
	rbTree.insert(40);
	rbTree.insert(22);
	rbTree.printTree();
	std::cout << std::endl << "After deleting an element" << std::endl;
	rbTree.deleteNode(40);
	rbTree.printTree();
}
