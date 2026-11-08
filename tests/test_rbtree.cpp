/*
 * Copyright (c) 2018-2026 Heng Yuan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <iostream>
#include <set>

#include <cookmem/rbtree.h>

#include "unittest.h"

static int
validateSubtree (
    const cookmem::PtrRBNode* node,
    const cookmem::PtrRBNode* parent,
    const cookmem::PtrRBNode* minNode,
    const cookmem::PtrRBNode* maxNode,
    std::size_t& nodeCount)
{
    if (node == nullptr)
    {
        return 1;
    }

    ASSERT_EQ (parent, node->getParent ());
    ASSERT_EQ (false, node->isInList ());
    ++nodeCount;

    // Check the DLL of same sized nodes.  The prev pointer of a list node
    // is its left child pointer.
    const cookmem::PtrRBNode* prevNode = node;
    for (const cookmem::PtrRBNode* listNode = node->getNext ();
         listNode != nullptr;
         listNode = listNode->getNext ())
    {
        ASSERT_EQ (true, listNode->isInList ());
        ASSERT_EQ (prevNode, listNode->getLeft ());
        ASSERT_EQ (node->getValue (), listNode->getValue ());
        ++nodeCount;
        prevNode = listNode;
    }

    if (minNode != nullptr)
    {
        ASSERT_EQ (true, minNode->getValue () < node->getValue ());
    }
    if (maxNode != nullptr)
    {
        ASSERT_EQ (true, node->getValue () < maxNode->getValue ());
    }
    if (node->isRed ())
    {
        ASSERT_EQ (false, node->getLeft () != nullptr && node->getLeft ()->isRed ());
        ASSERT_EQ (false, node->getRight () != nullptr && node->getRight ()->isRed ());
    }

    const int leftBlackHeight = validateSubtree (
        node->getLeft (), node, minNode, node, nodeCount);
    const int rightBlackHeight = validateSubtree (
        node->getRight (), node, node, maxNode, nodeCount);
    ASSERT_EQ (leftBlackHeight, rightBlackHeight);

    return leftBlackHeight + (node->isRed () ? 0 : 1);
}

/**
 * Validate the red-black tree properties, the parent pointers and the DLLs
 * of same sized nodes.
 *
 * @return  the number of nodes in the tree, including the list nodes.
 */
static std::size_t
validateTree (cookmem::PtrRBTree& tree)
{
    const cookmem::PtrRBNode* root = tree.getRoot ();
    if (root != nullptr)
    {
        ASSERT_EQ (false, root->isRed ());
    }
    std::size_t nodeCount = 0;
    validateSubtree (root, nullptr, nullptr, nullptr, nodeCount);
    return nodeCount;
}

static void
shuffleIndices (int* indices, int count, unsigned int& seed)
{
    for (int i = count - 1; i > 0; --i)
    {
        seed = seed * 1664525u + 1013904223u;
        const int j = seed % (i + 1);
        const int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

static void
test1 ()
{
    cookmem::PtrRBNode n[20];
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_EQ (false, list.contains (&n[i]));
    }

    list.add (&n[0], 10);
    list.add (&n[1], 20);
    list.add (&n[2], 30);
    list.add (&n[3], 40);
    list.add (&n[4], 50);
    list.add (&n[5], 20);
    list.add (&n[6], 40);
    list.add (&n[7], 80);
    list.add (&n[8], 150);
    list.add (&n[9], 250);
    list.add (&n[10], 60);
    list.add (&n[11], 220);
    list.add (&n[12], 330);
    list.add (&n[13], 440);
    list.add (&n[14], 550);
    list.add (&n[15], 320);
    list.add (&n[16], 340);
    list.add (&n[17], 430);
    list.add (&n[18], 10);
    list.add (&n[19], 20);

    ASSERT_EQ (false, list.isEmpty ());
    // list.printGraph();

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_EQ (true, list.contains (&n[i]));
    }

    ASSERT_EQ (&n[8], list.remove (size = 100));
    ASSERT_EQ (false, list.contains (&n[8]));

    ASSERT_EQ (&n[14], list.remove (size = 550));
    ASSERT_EQ (false, list.contains (&n[14]));

    ASSERT_EQ (&n[19], list.remove (size = 20));
    ASSERT_EQ (false, list.contains (&n[19]));
    ASSERT_EQ (&n[5], list.remove (size = 20));
    ASSERT_EQ (false, list.contains (&n[5]));
    ASSERT_EQ (&n[1], list.remove (size = 20));
    ASSERT_EQ (false, list.contains (&n[1]));
    ASSERT_EQ (&n[7], list.remove (size = 80));
    ASSERT_EQ (&n[2], list.remove (size = 25));
    ASSERT_EQ (nullptr, list.remove (size = 10000));
    ASSERT_EQ (&n[15], list.remove (size = 300));
    ASSERT_EQ (&n[12], list.remove (size = 300));
    ASSERT_EQ (&n[16], list.remove (size = 300));
    ASSERT_EQ (&n[17], list.remove (size = 300));
    ASSERT_EQ (&n[4], list.remove (size = 45));
    ASSERT_EQ (&n[10], list.remove (size = 45));
    ASSERT_EQ (&n[13], list.remove (size = 300));
    ASSERT_EQ (&n[11], list.remove (size = 200));
    ASSERT_EQ (std::size_t (220), size);
    ASSERT_EQ (&n[9], list.remove (size = 200));
    ASSERT_EQ (&n[18], list.remove (size = 10));
    ASSERT_EQ (&n[0], list.remove (size = 10));
    ASSERT_EQ (&n[6], list.remove (size = 10));
    ASSERT_EQ (&n[3], list.remove (size = 10));

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));
}

static void
test2 ()
{
    cookmem::PtrRBNode n[1];
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    list.add (&n[0], 10);
    list.remove ((size = 10));
}

static void
test3 ()
{
    cookmem::PtrRBNode n[1];
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    list.add (&n[0], 10);
    list.remove (&n[0]);
}

static void
test4 ()
{
    cookmem::PtrRBNode n[20];
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_EQ (false, list.contains (&n[i]));
    }

    list.add (&n[0], 10);
    list.add (&n[1], 20);
    list.add (&n[2], 30);
    list.add (&n[3], 40);
    list.add (&n[4], 50);
    list.add (&n[5], 20);
    list.add (&n[6], 40);
    list.add (&n[7], 80);
    list.add (&n[8], 150);
    list.add (&n[9], 250);
    list.add (&n[10], 60);
    list.add (&n[11], 220);
    list.add (&n[12], 330);
    list.add (&n[13], 440);
    list.add (&n[14], 550);
    list.add (&n[15], 320);
    list.add (&n[16], 340);
    list.add (&n[17], 430);
    list.add (&n[18], 10);
    list.add (&n[19], 20);

    list.remove (&n[8]);
    list.remove (&n[14]);
    list.remove (&n[19]);
    list.remove (&n[5]);
    list.remove (&n[1]);
    list.remove (&n[7]);
    list.remove (&n[2]);
    list.remove (&n[15]);
    list.remove (&n[12]);
    list.remove (&n[16]);
    list.remove (&n[17]);
    list.remove (&n[4]);
    list.remove (&n[10]);
    list.remove (&n[13]);
    list.remove (&n[11]);
    list.remove (&n[9]);
    list.remove (&n[18]);
    list.remove (&n[0]);
    list.remove (&n[6]);
    list.remove (&n[3]);

    ASSERT_EQ (true, list.isEmpty ());
}

static void
test5 ()
{
    cookmem::PtrRBNode n[3];
    cookmem::PtrRBNode outsider;
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    list.add (&n[0], 10);
    list.add (&n[1], 10);
    list.add (&n[2], 10);
    outsider.init (10, nullptr);
    ASSERT_EQ (false, list.contains (&outsider));

    list.remove (&n[0]);
    ASSERT_EQ (false, list.contains (&n[0]));
    ASSERT_EQ (true, list.contains (&n[1]));
    ASSERT_EQ (true, list.contains (&n[2]));

    ASSERT_EQ (&n[1], list.remove (size = 0));
    ASSERT_EQ (std::size_t (10), size);
    ASSERT_EQ (false, list.contains (&n[1]));
    list.remove (&n[2]);
    ASSERT_EQ (true, list.isEmpty ());
}

static void
test6 ()
{
    cookmem::PtrRBNode n[4];
    cookmem::PtrRBTree tree;

    tree.add (&n[0], 10);
    tree.add (&n[1], 10);
    tree.add (&n[2], 10);
    tree.add (&n[3], 10);

    tree.remove (&n[2]);
    ASSERT_EQ (false, tree.contains (&n[2]));
    ASSERT_EQ (true, tree.contains (&n[1]));
    ASSERT_EQ (true, tree.contains (&n[3]));

    tree.remove (&n[1]);
    ASSERT_EQ (false, tree.contains (&n[1]));
    tree.remove (&n[3]);
    ASSERT_EQ (true, tree.contains (&n[0]));
    tree.remove (&n[0]);
    ASSERT_EQ (true, tree.isEmpty ());

    cookmem::PtrRBNode withChildren[4];
    cookmem::PtrRBTree treeWithDuplicates;
    treeWithDuplicates.add (&withChildren[0], 20);
    treeWithDuplicates.add (&withChildren[1], 10);
    treeWithDuplicates.add (&withChildren[2], 30);
    treeWithDuplicates.add (&withChildren[3], 20);
    treeWithDuplicates.remove (&withChildren[0]);
    ASSERT_EQ (&withChildren[3], treeWithDuplicates.getRoot ());
    ASSERT_EQ (&withChildren[3], withChildren[1].getParent ());
    ASSERT_EQ (&withChildren[3], withChildren[2].getParent ());
    validateTree (treeWithDuplicates);
}

static void
test7 ()
{
    const int nodeCount = 31;
    cookmem::PtrRBNode nodes[nodeCount];
    bool present[nodeCount] = {};
    cookmem::PtrRBTree tree;
    unsigned int seed = 0xC001D00Du;
    int insertionOrder[nodeCount];
    int removalOrder[nodeCount];

    for (int round = 0; round < 16; ++round)
    {
        for (int i = 0; i < nodeCount; ++i)
        {
            insertionOrder[i] = i;
            removalOrder[i] = i;
        }
        shuffleIndices (insertionOrder, nodeCount, seed);
        shuffleIndices (removalOrder, nodeCount, seed);

        for (int i = 0; i < nodeCount; ++i)
        {
            const int index = insertionOrder[i];
            tree.add (&nodes[index], index + 1);
            present[index] = true;
            validateTree (tree);
        }

        for (int i = 0; i < nodeCount; ++i)
        {
            const int index = removalOrder[i];
            tree.remove (&nodes[index]);
            present[index] = false;
            validateTree (tree);
            for (int j = 0; j < nodeCount; ++j)
            {
                ASSERT_EQ (present[j], tree.contains (&nodes[j]));
            }
        }
    }
    ASSERT_EQ (true, tree.isEmpty ());
}

/**
 * Random mix of add, remove by size, remove by pointer and contains,
 * checked against a std::multiset.  Small key ranges create long lists of
 * same sized nodes, while large key ranges create mostly unique nodes.
 */
static void
test8 ()
{
    const int nodeCount = 256;
    const std::size_t keyRanges[] = { 4, 16, 64, 100000 };
    cookmem::PtrRBNode nodes[nodeCount];
    unsigned int seed = 0x5EEDF00Du;

    for (std::size_t keyRange : keyRanges)
    {
        for (int round = 0; round < 4; ++round)
        {
            cookmem::PtrRBTree tree;
            std::multiset<std::size_t> expected;
            bool present[nodeCount] = {};
            int freeNodes[nodeCount];
            int numFree = nodeCount;
            int usedNodes[nodeCount];
            int numUsed = 0;

            for (int i = 0; i < nodeCount; ++i)
            {
                freeNodes[i] = i;
            }

            for (int step = 0; step < 5000; ++step)
            {
                seed = seed * 1664525u + 1013904223u;
                const unsigned int op = (seed >> 16) % 10;
                seed = seed * 1664525u + 1013904223u;
                const unsigned int r = seed >> 8;

                if (op < 5 && numFree > 0)
                {
                    // add
                    const int index = freeNodes[--numFree];
                    const std::size_t value = 1 + r % keyRange;
                    tree.add (&nodes[index], value);
                    expected.insert (value);
                    present[index] = true;
                    usedNodes[numUsed++] = index;
                }
                else if (op < 8 && numUsed > 0)
                {
                    // remove by pointer
                    const int slot = r % numUsed;
                    const int index = usedNodes[slot];
                    usedNodes[slot] = usedNodes[--numUsed];

                    ASSERT_EQ (true, tree.contains (&nodes[index]));
                    const std::size_t value = nodes[index].getValue ();
                    tree.remove (&nodes[index]);
                    ASSERT_EQ (false, tree.contains (&nodes[index]));

                    expected.erase (expected.find (value));
                    present[index] = false;
                    freeNodes[numFree++] = index;
                }
                else
                {
                    // remove by size (best fit)
                    const std::size_t key = 1 + r % (keyRange + 2);
                    std::size_t size = key;
                    cookmem::PtrRBNode* node = tree.remove (size);
                    std::multiset<std::size_t>::iterator it = expected.lower_bound (key);
                    if (it == expected.end ())
                    {
                        ASSERT_EQ (nullptr, node);
                    }
                    else
                    {
                        ASSERT_EQ (true, node != nullptr);
                        ASSERT_EQ (*it, size);
                        ASSERT_EQ (size, node->getValue ());

                        const int index = static_cast<int> (node - nodes);
                        ASSERT_EQ (true, present[index]);
                        ASSERT_EQ (false, tree.contains (node));

                        expected.erase (it);
                        present[index] = false;
                        for (int i = 0; i < numUsed; ++i)
                        {
                            if (usedNodes[i] == index)
                            {
                                usedNodes[i] = usedNodes[--numUsed];
                                break;
                            }
                        }
                        freeNodes[numFree++] = index;
                    }
                }

                ASSERT_EQ (expected.size (), validateTree (tree));
            }
        }
    }
}

/**
 * canGrowInPlace: a node can grow only up to (not including) the next
 * larger value, and never when it has a list or is in a list.
 */
static void
test9 ()
{
    const int nodeCount = 15;
    cookmem::PtrRBNode nodes[nodeCount];
    cookmem::PtrRBNode duplicate;
    cookmem::PtrRBTree tree;
    int insertionOrder[nodeCount];
    unsigned int seed = 0x6A0Du;

    for (int i = 0; i < nodeCount; ++i)
    {
        insertionOrder[i] = i;
    }
    shuffleIndices (insertionOrder, nodeCount, seed);

    // values are 100, 200, ..., 1500
    for (int i = 0; i < nodeCount; ++i)
    {
        const int index = insertionOrder[i];
        tree.add (&nodes[index], (index + 1) * 100);
    }
    validateTree (tree);

    // This covers both nodes with a right subtree and nodes whose next
    // larger value is an ancestor.
    for (int i = 0; i < nodeCount - 1; ++i)
    {
        const std::size_t nextValue = (i + 2) * 100;
        ASSERT_EQ (true, tree.canGrowInPlace (&nodes[i], nextValue - 1));
        ASSERT_EQ (false, tree.canGrowInPlace (&nodes[i], nextValue));
    }
    // The largest node can grow to any value.
    ASSERT_EQ (true, tree.canGrowInPlace (&nodes[nodeCount - 1], 1000000));

    // Nodes with the same value are kept in a list, so neither the tree node
    // nor the list node can grow in place.
    tree.add (&duplicate, 300);
    ASSERT_EQ (false, tree.canGrowInPlace (&nodes[2], 350));
    ASSERT_EQ (false, tree.canGrowInPlace (&duplicate, 350));
    tree.remove (&duplicate);
    ASSERT_EQ (true, tree.canGrowInPlace (&nodes[2], 350));

    // The memory pool grows a node by updating the size field of the chunk,
    // which is the value of the node (the second word).  The tree must stay
    // valid afterward.
    reinterpret_cast<std::size_t*> (&nodes[2])[1] = 350;
    ASSERT_EQ (std::size_t (350), nodes[2].getValue ());
    ASSERT_EQ (std::size_t (nodeCount), validateTree (tree));

    std::size_t size = 301;
    ASSERT_EQ (&nodes[2], tree.remove (size));
    ASSERT_EQ (std::size_t (350), size);
    validateTree (tree);
}

unittest::TestFunction s_tests[] =
{
    test1,
    test2,
    test3,
    test4,
    test5,
    test6,
    test7,
    test8,
    test9,
};

int
main (int argc, const char* argv[])
{
    try
    {
        std::size_t numTests = sizeof (s_tests) / sizeof (unittest::TestFunction);
        for (std::size_t i = 0; i < numTests; ++i)
        {
            s_tests[i] ();
        }
    }
    catch (unittest::UnitTestException& ex)
    {
        printf ("Exception thrown at %s:%d.\n", ex.getFile (), ex.getLine ());
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        printf ("Exception thrown at %s:%d.\n", ex.getFile (), ex.getLine ());
        return 1;
    }
    return 0;
}
