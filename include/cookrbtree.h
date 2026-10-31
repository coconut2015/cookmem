/*
 * Copyright (c) 2018-2021 Heng Yuan
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
#ifndef COOK_PTR_RBTREE_H
#define COOK_PTR_RBTREE_H

#include <iostream>
#include <string>

#include "cookexception.h"

namespace cookmem
{

class PtrRBNode
{
public:
    typedef PtrRBNode       SelfType;
    typedef std::size_t     ValueType;

    void
    init (ValueType value, SelfType* parentNode)
    {
        m_size = value;
        m_parent = parentNode;
        m_children[0] = nullptr;
        m_children[1] = nullptr;
        m_next = nullptr;
        m_red = true;
    }

    ValueType
    getValue () const
    {
        return m_size;
    }

    SelfType**
    getChildren ()
    {
        return m_children;
    }

    SelfType* const *
    getChildren () const
    {
        return m_children;
    }

    SelfType*
    getLeft ()
    {
        return m_children[0];
    }

    const SelfType*
    getLeft () const
    {
        return m_children[0];
    }

    void
    setLeft (SelfType* left)
    {
        m_children[0] = left;
    }

    SelfType*
    getRight ()
    {
        return m_children[1];
    }

    const SelfType*
    getRight () const
    {
        return m_children[1];
    }

    void
    setRight (SelfType* right)
    {
        m_children[1] = right;
    }

    SelfType*
    getParent ()
    {
        return m_parent;
    }

    const SelfType*
    getParent () const
    {
        return m_parent;
    }

    void
    setParent (SelfType* parentNode)
    {
        m_parent = parentNode;
    }

    void
    addList (SelfType* other)
    {
        other->m_next = m_next;
        m_next = other;
    }

    void
    takeListFrom (SelfType* other)
    {
        m_next = other->m_next;
        other->m_next = nullptr;
        for (SelfType* node = m_next; node != nullptr; node = node->m_next)
        {
            node->m_parent = this;
        }
    }

    bool
    hasList ()
    {
        return m_next != nullptr;
    }

    SelfType*
    getNext ()
    {
        return m_next;
    }

    const SelfType*
    getNext () const
    {
        return m_next;
    }

    SelfType*
    removeFromList ()
    {
        SelfType*   retNode = m_next;
        SelfType*   nextNode = m_next->m_next;
        m_next = nextNode;
        retNode->m_next = nullptr;

        return retNode;
    }

    void
    removeFromList (SelfType* node)
    {
        SelfType*   prevNode = this;
        for (SelfType* currNode = m_next; currNode != nullptr; currNode = currNode->m_next)
        {
            if (currNode == node)
            {
                prevNode->m_next = currNode->m_next;
                currNode->m_next = nullptr;
                return;
            }
            prevNode = currNode;
        }
        COOKMEM_ASSERT (false);
    }

    bool
    isRed () const
    {
        return m_red;
    }

    void
    setRed (bool red)
    {
        m_red = red;
    }

    void
    setAsBlack ()
    {
        m_red = false;
    }

    void
    setAsRed ()
    {
        m_red = true;
    }

private:
    /** reserved to make the header the same as MemChunk in cookmem.h */
    std::size_t     m_reserved;
    /** size of the pointer */
    std::size_t     m_size;
    /** children */
    SelfType*       m_children[2];
    /** parent */
    SelfType*       m_parent;
    /** SLL of same sized nodes. */
    SelfType*       m_next;
    /** Whether this node is red. */
    bool            m_red;
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
    typedef Node                            NodeType;
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
            newNode->setAsBlack ();
            return;
        }
        if (newNode->getParent ()->getParent () == nullptr)
        {
            return;
        }

        balanceInsert (newNode);
    }
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
     * @return  the node matching the search key that is at least the size
     *          of the provided.  This node is also removed from the tree.
     */
    NodeType*
    remove (ValueType& size)
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
        NodeType*   targetNode = reinterpret_cast<Node*>(ptr);
        ValueType   key = targetNode->getValue ();

        NodeType*   node = m_root;

        while (node != nullptr)
        {
            if (node->getValue () < key)
            {
                node = node->getRight ();
            }
            else if (node->getValue () > key)
            {
                node = node->getLeft ();
            }
            else
            {
                break;
            }
        }

        COOKMEM_ASSERT (node != nullptr);

        if (node != targetNode)
        {
            COOKMEM_ASSERT (node->hasList ());

            node->removeFromList (targetNode);
            return;
        }

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
                    uncleNode->setAsBlack ();
                    node->getParent ()->setAsBlack ();
                    grandParentNode->setAsRed ();
                    node = node->getParent ()->getParent ();
                }
                else
                {
                    if (node == node->getParent ()->getRight ())
                    {
                        node = node->getParent ();
                        doLeftRotation(node);
                    }
                    node->getParent ()->setAsBlack ();
                    node->getParent ()->getParent ()->setAsRed ();
                    doRightRotation (node->getParent ()->getParent ());
                }
            }
            else
            {
                NodeType* uncleNode = grandParentNode->getLeft ();
                if (uncleNode != nullptr &&
                    uncleNode->isRed ())
                {
                    uncleNode->setAsBlack ();
                    node->getParent ()->setAsBlack ();
                    grandParentNode->setAsRed ();
                    node = node->getParent ()->getParent ();
                }
                else
                {
                    if (node == node->getParent ()->getLeft ())
                    {
                        node = node->getParent ();
                        doRightRotation (node);
                    }
                    node->getParent ()->setAsBlack ();
                    node->getParent ()->getParent ()->setAsRed ();
                    doLeftRotation (node->getParent ()->getParent ());
                }
            }

            if (node == m_root)
            {
                break;
            }
        }

        m_root->setAsBlack ();
    }

    void
    removeNode (NodeType* node)
    {
        if (node->hasList ())
        {
            NodeType* replacementNode = node->removeFromList ();
            NodeType* parentNode = node->getParent ();
            NodeType* leftNode = node->getLeft ();
            NodeType* rightNode = node->getRight ();
            bool red = node->isRed ();

            replacementNode->init (node->getValue (), parentNode);
            replacementNode->takeListFrom (node);
            replacementNode->setLeft (leftNode);
            replacementNode->setRight (rightNode);
            replacementNode->setRed (red);
            if (leftNode != nullptr)
            {
                leftNode->setParent (replacementNode);
            }
            if (rightNode != nullptr)
            {
                rightNode->setParent (replacementNode);
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
            childNode->setAsBlack ();
        }
        if (m_root != nullptr)
        {
            m_root->setAsBlack ();
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
                    siblingNode->setAsBlack ();
                    parentNode->setAsRed ();
                    doLeftRotation (parentNode);
                    siblingNode = parentNode->getRight ();
                }

                if (siblingNode == nullptr ||
                    (isBlack (siblingNode->getLeft ()) &&
                     isBlack (siblingNode->getRight ())))
                {
                    if (siblingNode != nullptr)
                    {
                        siblingNode->setAsRed ();
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
                            siblingNode->getLeft ()->setAsBlack ();
                        }
                        siblingNode->setAsRed ();
                        doRightRotation (siblingNode);
                        siblingNode = parentNode->getRight ();
                    }
                    siblingNode->setRed (parentNode->isRed ());
                    parentNode->setAsBlack ();
                    if (siblingNode->getRight () != nullptr)
                    {
                        siblingNode->getRight ()->setAsBlack ();
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
                    siblingNode->setAsBlack ();
                    parentNode->setAsRed ();
                    doRightRotation (parentNode);
                    siblingNode = parentNode->getLeft ();
                }

                if (siblingNode == nullptr ||
                    (isBlack (siblingNode->getLeft ()) &&
                     isBlack (siblingNode->getRight ())))
                {
                    if (siblingNode != nullptr)
                    {
                        siblingNode->setAsRed ();
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
                            siblingNode->getRight ()->setAsBlack ();
                        }
                        siblingNode->setAsRed ();
                        doLeftRotation (siblingNode);
                        siblingNode = parentNode->getLeft ();
                    }
                    siblingNode->setRed (parentNode->isRed ());
                    parentNode->setAsBlack ();
                    if (siblingNode->getLeft () != nullptr)
                    {
                        siblingNode->getLeft ()->setAsBlack ();
                    }
                    doRightRotation (parentNode);
                    node = m_root;
                    parentNode = nullptr;
                }
            }
        }

        if (node != nullptr)
        {
            node->setAsBlack ();
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

typedef RedBlackTree<PtrRBNode> PtrRBTree;

} // namespace cookmem

#endif // COOK_PTR_RBTREE_H
