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
#ifndef COOK_PTRAVLTREE_H_
#define COOK_PTRAVLTREE_H_

#include <cookutils.h>
#include <cstdio>

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
        /** parent node */
        Node*           parent;
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
     * Note that the size of the pointer must be at least (7 * size_t).
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

        m_root = insert (m_root, node);
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
        RemoveStruct rs (size);
        m_root = remove (m_root, rs);
        if (rs.node)
        {
            size = rs.node->size;
        }
        return rs.node;
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

        if (node->next)
        {
            // we have a DLL, but node to be removed the head of the DLL
            Node* next = node->next;
            Node* parent = node->parent;

            next->left = node->left;
            next->right = node->right;
            next->parent = parent;
            next->height = node->height;

            if (parent->left == node)
            {
                parent->left = next;
            }
            else
            {
                parent->right = next;
            }
            return;
        }

        // The node is not in a DLL.  So this is a more complicate case that
        // need to do the removal from the root (since there is a potential
        // need to balance the tree from root.
        std::size_t size = node->size;
        remove(size);
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
    inline static std::uint16_t max (std::uint16_t a, std::uint16_t b)
    {
        return (a < b) ? b : a;
    }

    inline static std::int16_t getHeight (const Node* node)
    {
        return node ? node->height : 0;
    }

    inline static void updateHeight (Node* node)
    {
        node->height = max (getHeight (node->left), getHeight (node->right)) + 1;
    }

    inline static void setLeftChild (Node* root, Node* left)
    {
        root->left = left;
        if (left)
        {
            left->parent = root;
        }
    }

    inline static void setRightChild (Node* root, Node* right)
    {
        root->right = right;
        if (right)
        {
            right->parent = root;
        }
    }

    inline static Node* insert (Node* root, Node* node)
    {
        if (root == nullptr)
        {
            node->left = nullptr;
            node->right = nullptr;
            node->parent = nullptr;
            node->next = nullptr;
            node->height = 1;
            return node;
        }

        if (node->size < root->size)
        {
            Node* left = insert (root->left, node);
            setLeftChild (root, left);
        }
        else if (node->size > root->size)
        {
            Node* right = insert (root->right, node);
            setRightChild (root, right);
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
            // We intentionally leave node->right, parent untouched
            // since they will not be used.
            return root;
        }
        return balance (root);
    }

    inline static Node* findMin (Node* root)
    {
        while (root->left)
        {
            root = root->left;
        }
        return root;
    }

    inline static Node* remove(Node* root, RemoveStruct& rs)
    {
        if (root == nullptr)
        {
            return nullptr;
        }

        if (rs.size < root->size)
        {
            setLeftChild (root, remove (root->left, rs));

            if (rs.node)
            {
                goto REMOVE_BALANCE;
            }
        }
        else if (rs.size > root->size)
        {
            setRightChild (root, remove (root->right, rs));

            goto REMOVE_BALANCE;
        }

        // At this point, this node's size is the smallest size
        // larger than or equal to rs.size.
        if (root->next)
        {
            // This is a simple case of having SLL of similar sized nodes.
            // just remove one from the SLL.
            Node* next = root->next;
            root->next = next->next;
            rs.node = next;
            return root;
        }

        {
            Node* left = root->left;
            Node* right = root->right;

            rs.node = root;

            if (left == nullptr)
            {
                return right;
            }
            else if (right == nullptr)
            {
                return left;
            }
            else
            {
                // this is a more complicated case.
                // the solution is the move the smallest of the right branch
                // here.
                Node* min = findMin (right);
                if (min == right)
                {
                    root = right;
                    setLeftChild (root, left);
                }
                else
                {
                    // first detach min from the parent
                    Node* parent = min->parent;
                    parent->left = nullptr;
                    if (parent != right)
                    {
                        Node* grandParent = parent->parent;
                        parent = balance (parent);
                        setLeftChild (grandParent, parent);
                    }
                    else
                    {
                        right = balance (right);
                    }

                    root = min;
                    root->parent = nullptr;
                    setLeftChild (root, left);
                    setRightChild (root, right);
                }
            }
        }

REMOVE_BALANCE:
        return balance (root);
    }

    inline static Node* balance (Node* root)
    {
        Node* left = root->left;
        Node* right = root->right;
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
        }
        updateHeight (root);
        return root;
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
        printf ("%016lx -- {", (std::size_t)root);
        if (root->left)
        {
            printf (" %016lx", (std::size_t)root->left);
        }
        if (root->right)
        {
            printf (" %016lx", (std::size_t)root->right);
        }
        printf (" }\n");
        printf ("%016lx [ label = %ld ]\n",
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

#endif  /* COOK_PTRAVLTREE_H_ */
