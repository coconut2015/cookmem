/*
 * Copyright (c) 2018 Heng Yuan
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
#ifndef COOK_PTRAVLTREE_H
#define COOK_PTRAVLTREE_H

#include <cstdio>
#include <cstdint>

#include <cookexception.h>

namespace cookmem
{

/**
 * This is an internally used AVL tree that uses the pointer passed in
 * store the information.
 */
class PtrAVLTree
{
private:
    /**
     * Internal tree node structure.
     */
    struct Node
    {
        /** reserved to make the header the same as MemChunk in cookmem.h */
        std::size_t     reserved;
        /** size of the pointer */
        std::size_t     size;
        /** left child */
        Node*           left;
        /** right child */
        Node*           right;
        /** DLL of same sized nodes.  The prev pointer if height is -1 is left.*/
        Node*           next;
        /** height of the node */
        std::int16_t    height;
    };

    /**
     * Internal structure for removal.
     *
     * We need a way to track the node being removed and size of the node.
     */
    struct RemoveStruct
    {
        /** the size to be searched */
        std::size_t     size;
        /** the node being removed */
        Node*           node;

        RemoveStruct (std::size_t s)
        : size(s), node(nullptr)
        {
        }
    };

public:
    /**
     * Default constructor.
     */
    PtrAVLTree()
    : m_root(nullptr)
    {
    }

    /**
     * Well, we do not do destructor maintenance.
     */
    ~PtrAVLTree() {}

    /**
     * Add a pointer to the search tree.
     *
     * Note that the size of the pointer must be at least (7 * sizeof(size_t)).
     *
     * @param   ptr
     *          the pointer to be added to the AVL tree.
     * @param   size
     *          the size of the pointer
     */
    void add(void* ptr, std::size_t size)
    {
        Node*   node = reinterpret_cast<Node*>(ptr);
        node->size = size;

        if (m_root == nullptr)
        {
            node->left = nullptr;
            node->right = nullptr;
            node->next = nullptr;
            node->height = 1;
            m_root = node;

            return;
        }

        Node* root;
        Node* stack[sizeof(Node*) * 8 + 1];
        std::int16_t depth;

        root = stack[0] = m_root;

        for (depth = 0; ; ++depth)
        {
            if (root == nullptr)
            {
                setParent (node, stack[depth - 1]);
                node->left = nullptr;
                node->right = nullptr;
                node->next = nullptr;
                node->height = 1;

                root = balance (stack, depth);
                if (root != nullptr)
                {
                    m_root = root;
                }
                return;
            }

            if (size < root->size)
            {
                root = stack[depth + 1] = root->left;
                continue;
            }
            else if (size > root->size)
            {
                root = stack[depth + 1] = root->right;
                continue;
            }
            else
            {
                // The current root matches the node's size.  In this case we
                // just add node to the next pointer of the root.
                //
                Node* next = root->next;
                root->next = node;
                node->next = next;
                node->left = root;
                node->height = -1;      // indicating that this node is in a DLL
                if (next)
                {
                    next->left = node;
                }

                // We can simply return since the tree structure is not
                // changed.
                return;
            }
        }
        throw Exception("Invalid PtrAVLTree.");
    }

    /**
     * Remove a node based on the size information.  The node obtained has
     * the size that at least the size obtained.
     *
     * @param [in,out]  size
     *          search key.  If the node is found, the actual size of the
     *          pointer is returned.
     * @return  the node matching the search key that is at least the size
     *          of the provided.  This node is also removed from the tree.
     */
    void* remove(std::size_t& size)
    {
        Node* root;
        Node* stack[sizeof(Node*) * 8 + 1];
        std::int16_t depth;

        if (m_root == nullptr)
        {
            return nullptr;
        }

        root = stack[0] = m_root;

        for (depth = 0; ; ++depth)
        {
            if (root == nullptr)
            {
                --depth;
                root = stack[depth];
                if (size > root->size)
                {
                    // check if the parent is also smaller.
                    if (depth == 0)
                    {
                        return nullptr;
                    }

                    --depth;
                    root = stack[depth];
                    if (size > root->size)
                    {
                        return nullptr;
                    }
                    break;
                }
                break;
            }

            if (size < root->size)
            {
                root = stack[depth + 1] = root->left;
                continue;
            }
            else if (size > root->size)
            {
                root = stack[depth + 1] = root->right;
                continue;
            }

            break;
        }

        // At this point, this node's size is the smallest size
        // larger than or equal to rs.size.
        if (root->next)
        {
            // This is a simple case of having SLL of similar sized nodes.
            // just remove one from the SLL.
            Node* next = root->next;
            root->next = next->next;

            size = next->size;
            return next;
        }

        Node* left = root->left;
        Node* right = root->right;

        Node* newRoot;
        if (left == nullptr)
        {
            newRoot = right;
        }
        else if (right == nullptr)
        {
            newRoot = left;
        }
        else
        {
            // this is a more complicated case.
            // the solution is the move the smallest of the right branch
            // here.
            std::int16_t newDepth;
            Node* newStack[sizeof(Node*) * 8 + 1];
            newStack[0] = right;
            for (newDepth = 0; newStack[newDepth]->left != nullptr; ++newDepth)
            {
                newStack[newDepth + 1] = newStack[newDepth]->left;
            }
            Node* min = newStack[newDepth];
            if (min == right)
            {
                newRoot = right;
                setLeftChild (newRoot, left);
            }
            else
            {
                // first detach min from the parent
                Node* parent = newStack[newDepth - 1];
                parent->left = nullptr;

                // rebalance the right
                Node* newRight = balance (newStack, newDepth);
                if (newRight != nullptr)
                {
                    right = newRight;
                }

                setLeftChild (min, left);
                setRightChild (min, right);

                newRoot = min;
            }
        }

        if (depth == 0)
        {
            m_root = newRoot;
            size = root->size;
            return root;
        }
        if (newRoot == nullptr)
        {
            // so root is the leaf.
            if (root->size < stack[depth - 1]->size)
            {
                stack[depth - 1]->left = nullptr;
            }
            else
            {
                stack[depth - 1]->right = nullptr;
            }
            newRoot = balance (stack, depth);
            if (newRoot != nullptr)
            {
                m_root = newRoot;
            }
            size = root->size;
            return root;
        }

        setParent (newRoot, stack[depth - 1]);

        stack[depth] = newRoot;
        newRoot = balance (stack, depth);

        if (newRoot != nullptr)
        {
            m_root = newRoot;
        }
        size = root->size;
        return root;
    }

    /**
     * Remove a node based on the pointer info.
     *
     * @param   ptr
     *          the pointer to be removed from the tree
     */
    void remove(void* ptr)
    {
        Node* node = reinterpret_cast<Node*>(ptr);

        if (node->height == -1)
        {
            // this is a simple case that node is in a DLL
            Node* prev = node->left;
            Node* next = node->next;
            prev->next = next;
            if (next)
            {
                next->left = prev;
            }
            return;
        }

        // The node is not in a DLL.  So this is a more complicate case that
        // need to do the removal from the root (since there is a potential
        // need to balance the tree from root.

        Node* root;
        Node* stack[sizeof(Node*) * 8 + 1];
        std::int16_t depth;

        std::size_t size = node->size;

        COOKMEM_ASSERT(m_root != nullptr);

        root = stack[0] = m_root;

        for (depth = 0; ; ++depth)
        {
            if (root == nullptr)
            {
                throw Exception("pointer not found.");
            }

            if (size < root->size)
            {
                root = stack[depth + 1] = root->left;
                continue;
            }
            else if (size > root->size)
            {
                root = stack[depth + 1] = root->right;
                continue;
            }

            break;
        }

        COOKMEM_ASSERT(root == node);

        if (root->next)
        {
            // Just use the next node to replace the current node.

            Node* next = root->next;
            next->left = root->left;
            next->right = root->right;
            next->height = root->height;

            if (depth == 0)
            {
                m_root = next;
            }
            else
            {
                setParent (next, stack[depth - 1]);
            }
            return;
        }

        Node* left = root->left;
        Node* right = root->right;

        Node* newRoot;
        if (left == nullptr)
        {
            newRoot = right;
        }
        else if (right == nullptr)
        {
            newRoot = left;
        }
        else
        {
            // this is a more complicated case.
            // the solution is the move the smallest of the right branch
            // here.
            std::int16_t newDepth;
            Node* newStack[sizeof(Node*) * 8 + 1];
            newStack[0] = right;
            for (newDepth = 0; newStack[newDepth]->left != nullptr; ++newDepth)
            {
                newStack[newDepth + 1] = newStack[newDepth]->left;
            }
            Node* min = newStack[newDepth];
            if (min == right)
            {
                newRoot = right;
                setLeftChild (newRoot, left);
            }
            else
            {
                // first detach min from the parent
                Node* parent = newStack[newDepth - 1];
                parent->left = nullptr;

                // rebalance the right
                Node* newRight = balance (newStack, newDepth - 1);
                if (newRight != nullptr)
                {
                    right = newRight;
                }

                setLeftChild (min, left);
                setRightChild (min, right);

                newRoot = min;
            }
        }

        if (depth == 0)
        {
            m_root = newRoot;
        }
        stack[depth] = newRoot;
        newRoot = balance (stack, depth);

        if (newRoot != nullptr)
        {
            m_root = newRoot;
        }
    }

    /**
     * Check if the tree is empty.
     *
     * @return  true if the tree is empty.  false otherwise.
     */
    bool isEmpty ()
    {
        return m_root == nullptr;
    }

    /**
     * Debugging function that prints the tree nodes to GraphViz format.
     */
    void printGraph ()
    {
        printf ("graph G {\n");
        printNode (m_root);
        printf ("}\n");
    }
private:
    inline static std::int16_t getMax (std::int16_t a, std::int16_t b)
    {
        return (a < b) ? b : a;
    }

    inline static std::int16_t getHeight (const Node* node)
    {
        return node ? node->height : 0;
    }

    inline static void updateHeight (Node* node)
    {
        node->height = getMax (getHeight (node->left), getHeight (node->right)) + 1;
    }

    inline static void setParent (Node* node, Node* parent)
    {
        if (node->size < parent->size)
        {
            parent->left = node;
        }
        else
        {
            parent->right = node;
        }
    }

    inline static void setLeftChild (Node* root, Node* left)
    {
        root->left = left;
    }

    inline static void setRightChild (Node* root, Node* right)
    {
        root->right = right;
    }

    inline static Node* balance (Node** stack, std::int16_t depth)
    {
        for (;;)
        {
            --depth;
            Node* root = stack[depth];
            Node* left = root->left;
            Node* right = root->right;

            std::int16_t oldHeight = root->height;
            int16_t diff = getHeight (left) - getHeight (right);
            if (diff > 1)
            {
                if (getHeight (left->left) >= getHeight (left->right))
                {
                    root = rotateWithLeftChild (root);
                }
                else
                {
                    root = doubleWithLeftChild (root);
                }
                // update the parent
                if (depth == 0)
                {
                    return root;
                }
                setParent (root, stack[depth - 1]);
                continue;
            }
            else if (diff < -1)
            {
                if (getHeight (right->right) >= getHeight (right->left))
                {
                    root = rotateWithRightChild (root);
                }
                else
                {
                    root = doubleWithRightChild (root);
                }
                // update the parent
                if (depth == 0)
                {
                    return root;
                }
                setParent (root, stack[depth - 1]);
                continue;
            }
            else
            {
                updateHeight (root);
            }

            if (depth == 0)
            {
                return root;
            }

            if (oldHeight == root->height)
                break;
        }
        return nullptr;
    }

    inline static Node* rotateWithLeftChild (Node* root)
    {
        Node* left = root->left;
        setLeftChild (root, left->right);
        setRightChild (left, root);
        updateHeight (root);
        updateHeight (left);
        return left;
    }

    inline static Node* rotateWithRightChild (Node* root)
    {
        Node* right = root->right;
        setRightChild (root, right->left);
        setLeftChild (right, root);
        updateHeight (root);
        updateHeight (right);
        return right;
    }

    inline static Node* doubleWithLeftChild (Node* root)
    {
        setLeftChild (root, rotateWithRightChild (root->left));
        return rotateWithLeftChild (root);
    }

    inline static Node* doubleWithRightChild (Node* root)
    {
        setRightChild (root, rotateWithLeftChild (root->right));
        return rotateWithRightChild (root);
    }

    static void printNode (Node* root)
    {
        if (!root)
            return;
        printf ("%016zx -- {", (std::size_t)root);
        if (root->left)
        {
            printf (" %016zx", (std::size_t)root->left);
        }
        if (root->right)
        {
            printf (" %016zx", (std::size_t)root->right);
        }
        printf (" }\n");
        printf ("%016zx [ label = %zd ]\n",
                (std::size_t)root,
                root->size);
        printNode (root->left);
        printNode (root->right);
    }

private:
    // node entries
    Node*       m_root;
};

}   // namespace cookmem

#endif  // COOK_PTRAVLTREE_H
