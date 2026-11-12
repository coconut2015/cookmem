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
#ifndef COOK_MEM_RBTREE_H
#define COOK_MEM_RBTREE_H

#include <cstddef>

#include "exception.h"

/// @cond INTERNAL
// Marks a private field that is intentionally never used, so that compilers
// do not warn about it.
#if defined(__GNUC__)
#define COOKMEM_UNUSED_FIELD __attribute__ ((unused))
#else
#define COOKMEM_UNUSED_FIELD
#endif
/// @endcond

namespace cookmem
{

/**
 * The node of RedBlackTree, which is stored in the memory being tracked.
 *
 * The memory pool places this node at the start of a free chunk, so its
 * layout must match the chunk header: the second word (m_size) is both the
 * value of the node and the size field of the chunk.  The memory must be at
 * least sizeof(PtrRBNode) (7 words) large.
 *
 * Nodes with the same value are kept in a doubly linked list (DLL) attached
 * to the one node that is in the tree.
 */
class PtrRBNode
{
public:
    /** The type of this node. */
    typedef PtrRBNode       SelfType;
    /** The type of the value (the size of the memory). */
    typedef std::size_t     ValueType;

    /**
     * Initialize the node as a red leaf.
     *
     * @param   value
     *          the value of the node.
     * @param   parentNode
     *          the parent node, or nullptr for the root.
     */
    void
    init (ValueType value, SelfType* parentNode)
    {
        m_size = value;
        m_parent = parentNode;
        m_children[0] = nullptr;
        m_children[1] = nullptr;
        m_next = nullptr;
        m_red = true;
        m_inList = false;
    }

    /** Get the value of the node. */
    ValueType
    getValue () const
    {
        return m_size;
    }

    /** Get the left and right child pointers as an array. */
    SelfType**
    getChildren ()
    {
        return m_children;
    }

    /** Get the left and right child pointers as an array. */
    SelfType* const *
    getChildren () const
    {
        return m_children;
    }

    /** Get the left child, or the previous node for a list node. */
    SelfType*
    getLeft ()
    {
        return m_children[0];
    }

    /** Get the left child, or the previous node for a list node. */
    const SelfType*
    getLeft () const
    {
        return m_children[0];
    }

    /** Set the left child. */
    void
    setLeft (SelfType* left)
    {
        m_children[0] = left;
    }

    /** Get the right child. */
    SelfType*
    getRight ()
    {
        return m_children[1];
    }

    /** Get the right child. */
    const SelfType*
    getRight () const
    {
        return m_children[1];
    }

    /** Set the right child. */
    void
    setRight (SelfType* right)
    {
        m_children[1] = right;
    }

    /** Get the parent node, or nullptr for the root. */
    SelfType*
    getParent ()
    {
        return m_parent;
    }

    /** Get the parent node, or nullptr for the root. */
    const SelfType*
    getParent () const
    {
        return m_parent;
    }

    /** Set the parent node. */
    void
    setParent (SelfType* parentNode)
    {
        m_parent = parentNode;
    }

    /**
     * Add a node of the same value to the DLL of this tree node.
     *
     * A list node uses m_children[0] as the pointer to its previous node,
     * which is either another list node or the tree node itself.  List
     * nodes do not use m_parent or m_children[1].
     */
    void
    addList (SelfType* other)
    {
        other->m_next = m_next;
        other->m_children[0] = this;
        other->m_inList = true;
        if (m_next != nullptr)
        {
            m_next->m_children[0] = other;
        }
        m_next = other;
    }

    /** Check if this tree node has a DLL of nodes with the same value. */
    bool
    hasList () const
    {
        return m_next != nullptr;
    }

    /**
     * Check if this node is in the DLL of a tree node, rather than being
     * a node of the tree itself.
     */
    bool
    isInList () const
    {
        return m_inList;
    }

    /** Get the next node in the DLL of nodes with the same value. */
    SelfType*
    getNext ()
    {
        return m_next;
    }

    /** Get the next node in the DLL of nodes with the same value. */
    const SelfType*
    getNext () const
    {
        return m_next;
    }

    /**
     * Remove the first node in the DLL of this tree node.  The list must
     * not be empty.
     *
     * @return  the node removed.
     */
    SelfType*
    removeFromList ()
    {
        SelfType*   retNode = m_next;
        SelfType*   nextNode = retNode->m_next;
        m_next = nextNode;
        if (nextNode != nullptr)
        {
            nextNode->m_children[0] = this;
        }
        retNode->m_next = nullptr;
        retNode->m_inList = false;

        return retNode;
    }

    /**
     * Unlink this list node from the DLL it is in.
     */
    void
    unlinkFromList ()
    {
        COOKMEM_ASSERT (m_inList);

        SelfType*   prevNode = m_children[0];
        SelfType*   nextNode = m_next;
        prevNode->m_next = nextNode;
        if (nextNode != nullptr)
        {
            nextNode->m_children[0] = prevNode;
        }
        m_next = nullptr;
        m_inList = false;
    }

    /**
     * Replace the tree node with the first node in its DLL.  The first
     * list node takes over the position, color and the rest of the list,
     * so this takes constant time.  The list must not be empty.
     *
     * @return  the replacement node, which is now a tree node.
     */
    SelfType*
    promoteFirstListNode ()
    {
        SelfType*   replacementNode = m_next;

        // The rest of the list stays attached to replacementNode, and the
        // first node of the rest already points back to replacementNode.
        replacementNode->m_parent = m_parent;
        replacementNode->m_children[0] = m_children[0];
        replacementNode->m_children[1] = m_children[1];
        replacementNode->m_red = m_red;
        replacementNode->m_inList = false;

        m_next = nullptr;
        return replacementNode;
    }

    /** Check if the node is red. */
    bool
    isRed () const
    {
        return m_red;
    }

    /** Set the node to be red or black. */
    void
    setRed (bool red)
    {
        m_red = red;
    }

private:
    /**
     * Reserved, so that the layout matches the header of a memory chunk:
     * this is the size of the previous chunk, which the tree does not use.
     */
    std::size_t     m_reserved COOKMEM_UNUSED_FIELD;
    /** size of the pointer */
    std::size_t     m_size;
    /** children */
    SelfType*       m_children[2];
    /** parent */
    SelfType*       m_parent;
    /** DLL of same sized nodes.  The prev pointer of a list node is m_children[0]. */
    SelfType*       m_next;
    /** Whether this node is red. */
    bool            m_red;
    /** Whether this node is in the DLL of a tree node. */
    bool            m_inList;
};

/**
 * This class defines a Red-Black Tree.
 *
 * @tparam  Node
 *          Any classes similar to PtrRBNode
 */
template<class Node>
class RedBlackTree
{
public:
    /** The type of the tree nodes. */
    typedef Node                            NodeType;
    /** The type of the node values. */
    typedef typename NodeType::ValueType    ValueType;

public:
    RedBlackTree ()
    : m_root (nullptr)
    {
    }

    /**
     * Add a pointer to the search tree.
     *
     * Note that the size of the pointer must be at least (7 * sizeof(size_t)).
     *
     * @param   ptr
     *          the pointer to be added to the red-black tree.
     * @param   value
     *          the size of the pointer
     */
    void
    add (void* ptr, ValueType value)
    {
        NodeType* parentNode = searchClosest (value);
        NodeType* newNode = reinterpret_cast<NodeType*>(ptr);
        newNode->init (value, parentNode);

        if (parentNode == nullptr)
        {
            m_root = newNode;
        }
        else if (value < parentNode->getValue ())
        {
            parentNode->setLeft (newNode);
        }
        else if (value > parentNode->getValue ())
        {
            parentNode->setRight (newNode);
        }
        else    // value == parentNode->getValue ()
        {
            // Equal values are kept in a list and do not affect tree balance.
            parentNode->addList (newNode);
            return;
        }

        if (newNode->getParent () == nullptr)
        {
            newNode->setRed (false);
            return;
        }
        if (newNode->getParent ()->getParent () == nullptr)
        {
            return;
        }

        balanceInsert (newNode);
    }
    /**
     * Get the root node.  This is mostly for testing.
     *
     * @return  the root node, or nullptr if the tree is empty.
     */
    NodeType* getRoot()
    {
        return this->m_root;
    }

    /**
     * Remove a node based on the size information.  The node obtained has
     * the size that at least the size obtained.
     *
     * @param [in,out]  size
     *                  search key.  If the node is found, the actual size of the
     *                  pointer is returned.
     * @param   limit
     *          the node must be smaller than this value.  Otherwise nothing
     *          is removed.
     * @return  the node matching the search key that is at least the size
     *          of the provided.  This node is also removed from the tree.
     */
    NodeType*
    remove (ValueType& size, ValueType limit = ~static_cast<ValueType> (0))
    {
        NodeType*   lastBigNode = nullptr;
        NodeType*   node = m_root;

        while (node != nullptr)
        {
            if (node->getValue () < size)
            {
                node = node->getRight ();
            }
            else if (node->getValue () > size)
            {
                lastBigNode = node;
                node = node->getLeft ();
            }
            else
            {
                break;
            }
        }

        if (node == nullptr)
        {
            if (lastBigNode == nullptr)
            {
                return nullptr;
            }
            else
            {
                node = lastBigNode;
            }
        }

        if (node->getValue () >= limit)
        {
            return nullptr;
        }

        if (node->hasList ())
        {
            NodeType* listNode = node->removeFromList ();
            size = node->getValue ();
            return listNode;
        }

        size = node->getValue ();
        removeNode (node);

        return node;
    }

    /**
     * Remove a node based on the pointer info.
     *
     * @param   ptr
     *          the pointer to be removed from the tree
     */
    void
    remove (void* ptr)
    {
        NodeType*   targetNode = reinterpret_cast<NodeType*>(ptr);

        if (targetNode->isInList ())
        {
            // The node is in the DLL of a tree node.  Just unlink it.
            targetNode->unlinkFromList ();
            return;
        }

        // The node is in the tree.  Since nodes have parent pointers, there
        // is no need to search from the root.
        removeNode (targetNode);
    }

    /**
     * Check if the pointer is stored in the tree.
     *
     * @param   ptr
     *          the pointer to be searched.
     */
    bool
    contains (void* ptr)
    {
        if (m_root == nullptr)
        {
            return false;
        }

        NodeType* node2 = reinterpret_cast<NodeType*>(ptr);
        const ValueType value = node2->getValue ();

        NodeType* node = m_root;
        while (node != nullptr)
        {
            if (value < node->getValue ())
            {
                node = node->getLeft ();
            }
            else if (value > node->getValue ())
            {
                node = node->getRight ();
            }
            else
            {
                do
                {
                    if (node == node2)
                    {
                        return true;
                    }
                    node = node->getNext ();
                }
                while (node != nullptr);
            }
        }
        return false;
    }

    /**
     * Check if a node in the tree can take a larger value without changing
     * its position in the tree.  If so, the caller can update the value in
     * place instead of removing the node and adding it back.
     *
     * @param   ptr
     *          the pointer stored in the tree.
     * @param   newValue
     *          the new value, which must be larger than the current value.
     * @return  true if the value can be updated in place.
     */
    bool
    canGrowInPlace (void* ptr, ValueType newValue)
    {
        NodeType* node = reinterpret_cast<NodeType*>(ptr);

        // All nodes in a list must have the same value.
        if (node->isInList () || node->hasList ())
        {
            return false;
        }

        // The new value must stay below the value of the next larger node.
        if (node->getRight () != nullptr)
        {
            return newValue < getLeftMostNode (node->getRight ())->getValue ();
        }

        NodeType* parentNode = node->getParent ();
        while (parentNode != nullptr && node == parentNode->getRight ())
        {
            node = parentNode;
            parentNode = node->getParent ();
        }
        return parentNode == nullptr || newValue < parentNode->getValue ();
    }

    /**
     * Check if the tree is empty.
     *
     * @return  true if the tree is empty.  false otherwise.
     */
    bool
    isEmpty () const
    {
        return m_root == nullptr;
    }

private:

    void
    updateParent (NodeType* newChildNode, NodeType* oldChildNode)
    {
        NodeType* parentNode = oldChildNode->getParent ();
        if (parentNode == nullptr)
        {
            m_root = newChildNode;
        }
        else if (oldChildNode == parentNode->getLeft ())
        {
            parentNode->setLeft (newChildNode);
        }
        else if (oldChildNode == parentNode->getRight ())
        {
            parentNode->setRight (newChildNode);
        }
        else
        {
            COOKMEM_ASSERT (false);
        }
        if (newChildNode != nullptr)
        {
            newChildNode->setParent (parentNode);
        }
    }

    NodeType*
    getLeftMostNode (NodeType* node)
    {
        while (node->getLeft () != nullptr)
        {
            node = node->getLeft ();
        }
        return node;
    }

    /**
     * Get the node that closest matching the key.
     *
     * @param   key
     *          The key value to search for.
     */
    NodeType*
    searchClosest (ValueType key)
    {
        if (m_root == nullptr)
        {
            return nullptr;
        }

        NodeType* node = m_root;

        for (;;)
        {
            if (key < node->getValue ())
            {
                if (node->getLeft () == nullptr)
                {
                    return node;
                }
                else
                {
                    node = node->getLeft ();
                }
            }
            else if (key > node->getValue ())
            {
                if (node->getRight () == nullptr)
                {
                    return node;
                }
                else
                {
                    node = node->getRight ();
                }
            }
            else
            {
                return node;
            }
        }
    }

    void
    balanceInsert (NodeType* node)
    {
        while (node != m_root &&
               node->getParent ()->isRed ())
        {
            NodeType* grandParentNode = node->getParent ()->getParent ();
            if (node->getParent () == grandParentNode->getLeft ())
            {
                NodeType* uncleNode = grandParentNode->getRight ();
                if (uncleNode != nullptr &&
                    uncleNode->isRed ())
                {
                    uncleNode->setRed (false);
                    node->getParent ()->setRed (false);
                    grandParentNode->setRed (true);
                    node = node->getParent ()->getParent ();
                }
                else
                {
                    if (node == node->getParent ()->getRight ())
                    {
                        node = node->getParent ();
                        doLeftRotation(node);
                    }
                    node->getParent ()->setRed (false);
                    node->getParent ()->getParent ()->setRed (true);
                    doRightRotation (node->getParent ()->getParent ());
                }
            }
            else
            {
                NodeType* uncleNode = grandParentNode->getLeft ();
                if (uncleNode != nullptr &&
                    uncleNode->isRed ())
                {
                    uncleNode->setRed (false);
                    node->getParent ()->setRed (false);
                    grandParentNode->setRed (true);
                    node = node->getParent ()->getParent ();
                }
                else
                {
                    if (node == node->getParent ()->getLeft ())
                    {
                        node = node->getParent ();
                        doRightRotation (node);
                    }
                    node->getParent ()->setRed (false);
                    node->getParent ()->getParent ()->setRed (true);
                    doLeftRotation (node->getParent ()->getParent ());
                }
            }

            if (node == m_root)
            {
                break;
            }
        }

        m_root->setRed (false);
    }

    void
    removeNode (NodeType* node)
    {
        if (node->hasList ())
        {
            // Replace the node with the first node of its list.  The tree
            // structure and colors do not change.
            NodeType* replacementNode = node->promoteFirstListNode ();
            if (replacementNode->getLeft () != nullptr)
            {
                replacementNode->getLeft ()->setParent (replacementNode);
            }
            if (replacementNode->getRight () != nullptr)
            {
                replacementNode->getRight ()->setParent (replacementNode);
            }
            updateParent (replacementNode, node);
            return;
        }

        NodeType* replacementNode = node;
        bool replacementWasRed = replacementNode->isRed ();
        NodeType* childNode;
        NodeType* childParentNode;

        if (node->getLeft () == nullptr)
        {
            childNode = node->getRight ();
            childParentNode = node->getParent ();
            updateParent (childNode, node);
        }
        else if (node->getRight () == nullptr)
        {
            childNode = node->getLeft ();
            childParentNode = node->getParent ();
            updateParent (childNode, node);
        }
        else
        {
            replacementNode = getLeftMostNode (node->getRight ());
            replacementWasRed = replacementNode->isRed ();
            childNode = replacementNode->getRight ();

            if (replacementNode->getParent () == node)
            {
                childParentNode = replacementNode;
                if (childNode != nullptr)
                {
                    childNode->setParent (replacementNode);
                }
            }
            else
            {
                childParentNode = replacementNode->getParent ();
                updateParent (childNode, replacementNode);
                replacementNode->setRight (node->getRight ());
                replacementNode->getRight ()->setParent (replacementNode);
            }

            updateParent (replacementNode, node);
            replacementNode->setLeft (node->getLeft ());
            replacementNode->getLeft ()->setParent (replacementNode);
            replacementNode->setRed (node->isRed ());
        }

        if (!replacementWasRed)
        {
            balanceRemove (childNode, childParentNode);
        }
        if (childNode != nullptr)
        {
            childNode->setRed (false);
        }
        if (m_root != nullptr)
        {
            m_root->setRed (false);
        }
    }

    static bool
    isBlack (const NodeType* node)
    {
        return node == nullptr || !node->isRed ();
    }

    void
    balanceRemove (NodeType* node, NodeType* parentNode)
    {
        while (node != m_root && isBlack (node))
        {
            if (parentNode == nullptr)
            {
                break;
            }

            if (node == parentNode->getLeft ())
            {
                NodeType* siblingNode = parentNode->getRight ();
                if (siblingNode != nullptr && siblingNode->isRed ())
                {
                    siblingNode->setRed (false);
                    parentNode->setRed (true);
                    doLeftRotation (parentNode);
                    siblingNode = parentNode->getRight ();
                }

                if (siblingNode == nullptr ||
                    (isBlack (siblingNode->getLeft ()) &&
                     isBlack (siblingNode->getRight ())))
                {
                    if (siblingNode != nullptr)
                    {
                        siblingNode->setRed (true);
                    }
                    node = parentNode;
                    parentNode = node->getParent ();
                }
                else
                {
                    if (isBlack (siblingNode->getRight ()))
                    {
                        if (siblingNode->getLeft () != nullptr)
                        {
                            siblingNode->getLeft ()->setRed (false);
                        }
                        siblingNode->setRed (true);
                        doRightRotation (siblingNode);
                        siblingNode = parentNode->getRight ();
                    }
                    siblingNode->setRed (parentNode->isRed ());
                    parentNode->setRed (false);
                    if (siblingNode->getRight () != nullptr)
                    {
                        siblingNode->getRight ()->setRed (false);
                    }
                    doLeftRotation (parentNode);
                    node = m_root;
                    parentNode = nullptr;
                }
            }
            else
            {
                NodeType* siblingNode = parentNode->getLeft ();
                if (siblingNode != nullptr && siblingNode->isRed ())
                {
                    siblingNode->setRed (false);
                    parentNode->setRed (true);
                    doRightRotation (parentNode);
                    siblingNode = parentNode->getLeft ();
                }

                if (siblingNode == nullptr ||
                    (isBlack (siblingNode->getLeft ()) &&
                     isBlack (siblingNode->getRight ())))
                {
                    if (siblingNode != nullptr)
                    {
                        siblingNode->setRed (true);
                    }
                    node = parentNode;
                    parentNode = node->getParent ();
                }
                else
                {
                    if (isBlack (siblingNode->getLeft ()))
                    {
                        if (siblingNode->getRight () != nullptr)
                        {
                            siblingNode->getRight ()->setRed (false);
                        }
                        siblingNode->setRed (true);
                        doLeftRotation (siblingNode);
                        siblingNode = parentNode->getLeft ();
                    }
                    siblingNode->setRed (parentNode->isRed ());
                    parentNode->setRed (false);
                    if (siblingNode->getLeft () != nullptr)
                    {
                        siblingNode->getLeft ()->setRed (false);
                    }
                    doRightRotation (parentNode);
                    node = m_root;
                    parentNode = nullptr;
                }
            }
        }

        if (node != nullptr)
        {
            node->setRed (false);
        }
    }

    /**
     * Do a left rotation such that the following structure
     *
     *        A
     *       / \
     *      B   C
     *         / \
     *        D   E
     *
     * becomes the following
     *
     *          C
     *         / \
     *        A   E
     *       / \
     *      B   D
     */
    void
    doLeftRotation (NodeType* nodeA)
    {
        NodeType* nodeC = nodeA->getRight ();

        NodeType* nodeD = nodeC->getLeft ();
        nodeA->setRight (nodeD);
        if (nodeD != nullptr)
        {
            nodeD->setParent (nodeA);
        }

        updateParent (nodeC, nodeA);

        nodeC->setLeft (nodeA);
        nodeA->setParent (nodeC);
    }

    /**
     * Do a right rotation such that the following structure
     *
     *          A
     *         / \
     *        B   C
     *       / \
     *      D   E
     *
     * becomes the following
     *
     *        B
     *       / \
     *      D   A
     *         / \
     *        E   C
     */
    void
    doRightRotation (NodeType* nodeA)
    {
        NodeType* nodeB = nodeA->getLeft ();

        NodeType* nodeE = nodeB->getRight ();
        nodeA->setLeft (nodeE);
        if (nodeE != nullptr)
        {
            nodeE->setParent (nodeA);
        }

        updateParent (nodeB, nodeA);

        nodeB->setRight (nodeA);
        nodeA->setParent (nodeB);
    }

private:
    NodeType *m_root;
};

/**
 * The red-black tree used for the large chunk bins of MemPool and for
 * CachedArena by default.
 */
typedef RedBlackTree<PtrRBNode> PtrRBTree;

} // namespace cookmem

#endif // COOK_MEM_RBTREE_H
