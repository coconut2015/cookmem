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

#include <cookmem/rbtree.h>

#include "unittest.h"

static int
validateSubtree (
    const cookmem::PtrRBNode* node,
    const cookmem::PtrRBNode* parent,
    const cookmem::PtrRBNode* minNode,
    const cookmem::PtrRBNode* maxNode)
{
    if (node == nullptr)
    {
        return 1;
    }

    ASSERT_EQ (parent, node->getParent ());
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
        node->getLeft (), node, minNode, node);
    const int rightBlackHeight = validateSubtree (
        node->getRight (), node, node, maxNode);
    ASSERT_EQ (leftBlackHeight, rightBlackHeight);

    return leftBlackHeight + (node->isRed () ? 0 : 1);
}

static void
validateTree (cookmem::PtrRBTree& tree)
{
    const cookmem::PtrRBNode* root = tree.getRoot ();
    if (root != nullptr)
    {
        ASSERT_EQ (false, root->isRed ());
    }
    validateSubtree (root, nullptr, nullptr, nullptr);
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

unittest::TestFunction s_tests[] =
{
    test1,
    test2,
    test3,
    test4,
    test5,
    test6,
    test7,
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
