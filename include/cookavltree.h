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
#ifndef COOK_AVLTREE_H
#define COOK_AVLTREE_H

#include <new>
#include <type_traits>
#include <iostream>
#include <cookutils.h>

namespace cookmem
{

/**
 * A simple comparator that compares to objects using their references.
 */
template<class T>
struct ObjectComparator
{
    /**
     * Compare two objects using object references.
     *
     * @param   t1
     *          Object 1
     * @param   t2
     *          Object 2
     * @return  0 for equal, -1 if t1 < t2, 1 if t1 > t2.
     */
    int compare (const T& t1, const T& t2)
    {
        if (t1 == t2)
        {
            return 0;
        }
        return t1 < t2 ? -1 : 1;
    }
};

/**
 * A simple comparator that compares to objects using their values.
 *
 * This comparator is better for comparing integers.
 */
template<class T>
struct ValueComparator
{
    /**
     * Compare two objects using object values.
     *
     * @param   t1
     *          Object 1
     * @param   t2
     *          Object 2
     * @return  the integer difference between t1 and t2.
     */
    int compare (T t1, T t2)
    {
        return (int)(t1 - t2);
    }
};


template<class Key, class KeyComparator, class Value, std::size_t TREESIZE>
class FixedLRU;

/**
 * This is a simple AVL tree implementation that uses up to a fixed sized
 * nodes.  The intent of of this tree is to eliminate memory allocation,
 * deallocation and fragmentation due to adding / removing tree nodes.
 *
 * Template parameters:
 *
 * Key - the key class type
 * KeyComparator - the functor for comparing keys
 * Value - the value class type
 * TREESIZE - the maximum size of the tree.
 */
template<class Key, class KeyComparator, class Value, std::size_t TREESIZE>
class FixedAVLTree
{
friend FixedLRU<Key, KeyComparator, Value, TREESIZE>;
public:
    /**
     * The signed integer type used to index the array.  Its value is based
     * on the value of TREESIZE.
     */
    typedef IntTypeSelector<TREESIZE>   IntType;

private:
    typedef typename std::aligned_storage<sizeof(Key),alignof(Key)>::type KeyType;
    typedef typename std::aligned_storage<sizeof(Value),alignof(Value)>::type ValueType;

    struct Node
    {
        KeyType     key;
        IntType     left;
        IntType     right;
        IntType     parent;
        IntType     height;
    };

    /**
     * Internal structure for insertion.
     */
    struct InsertStruct
    {
        const Key&      key;
        const Value&    value;
        IntType         pos;
        KeyComparator   c;

        InsertStruct (const Key& k, const Value& v)
        : key (k), value (v), pos (-1)
        {
        }

        int compare (const Key& k)
        {
            return c.compare (key, k);
        }
    };

    /**
     * Internal structure for removal.
     */
    struct RemoveStruct
    {
        const Key&      key;
        IntType         pos;
        KeyComparator   c;

        RemoveStruct (const Key& k)
        : key (k), pos (-1)
        {
        }

        int compare (const Key& k)
        {
            return c.compare (key, k);
        }
    };

public:
    /**
     * Default constructor.
     */
    FixedAVLTree()
    : m_root(-1), m_size(0), m_free(0), m_entries()
    {
        initEntries();
    }

    /**
     * Empty destructor
     */
    ~FixedAVLTree ()
    {
        clear();
    }

    /**
     * Clear the tree.
     */
    void clear()
    {
        clearNode (m_root);
        m_root = -1;
        m_size = 0;
    }

    /**
     * Add a key / value pair to the search tree.
     *
     * @param   key
     *          the key to be inserted / updated.
     * @param   value
     *          the value to be inserted / updated.
     * @return  the inserting / updating position. -1 if the insertion failed.
     */
    IntType add(const Key& key, const Value& value)
    {
        InsertStruct is (key, value);
        IntType index = insert (m_root, is);
        if (index < 0)
        {
            return -1;
        }
        m_root = index;
        return is.pos;
    }

    /**
     * Search the tree using the key provided.
     *
     * @param   key
     *          the search key
     * @return  the pointer to the value found.  NULL if not found.
     */
    Value* get(const Key& key)
    {
        IntType index = search(key);
        if (index < 0)
        {
            return NULL;
        }
        return reinterpret_cast<Value*>(&m_values[index]);
    }

    /**
     * Remove a node based on the search key provided.
     *
     * @param   key
     *          search key
     * @return  the removal position.  -1 if the node was not found.
     */
    IntType remove(const Key& key)
    {
        RemoveStruct rs (key);
        m_root = remove (m_root, rs);
        return rs.pos;
    }

    /**
     * The current size of the tree.
     *
     * @return  the size of the tree.
     */
    std::size_t size() const
    {
        return (std::size_t)m_size;
    }

    /**
     * Debugging function that prints the tree nodes to GraphViz format.
     */
    void printGraph ()
    {
        std::cout << "graph G {" << std::endl;
        printNode (m_root);
        std::cout << "}" << std::endl;
    }
private:
    void initEntries()
    {
        m_entries[0].left = TREESIZE - 1;
        m_entries[0].right = 1;
        for (IntType i = 1; i < TREESIZE - 1; ++i)
        {
            m_entries[i].left = i - 1;
            m_entries[i].right = i + 1;
        }
        m_entries[TREESIZE - 1].left = TREESIZE - 2;
        m_entries[TREESIZE - 1].right = 0;
    }

    void clearNode (IntType index)
    {
        if (index < 0)
            return;
        removeKV (index);
        clearNode (m_entries[index].left);
        clearNode (m_entries[index].right);
        addFreeIndex (index);
    }

    void removeKV (IntType index)
    {
        reinterpret_cast<Key*>(&m_entries[index].key)->~Key();
        reinterpret_cast<Value*>(&m_values[index])->~Value();
    }

    void updateValue (IntType index, const Value& v)
    {
        *reinterpret_cast<Value*>(&m_values[index]) = v;
    }

    void setValue (IntType index, const Key& k, const Value& v)
    {
        new (&m_entries[index]) Key (k);
        new (&m_values[index]) Value (v);
    }

    inline IntType max (IntType a, IntType b) const
    {
        return (a < b) ? b : a;
    }

    inline IntType getHeight (IntType index) const
    {
        return index < 0 ? 0 : m_entries[index].height;
    }

    inline void updateHeight (IntType index)
    {
        m_entries[index].height = max (getHeight (m_entries[index].left), getHeight (m_entries[index].right)) + 1;
    }

    inline void setLeftChild (IntType root, IntType left)
    {
        m_entries[root].left = left;
        if (left >= 0)
        {
            m_entries[left].parent = root;
        }
    }

    inline void setRightChild (IntType root, IntType right)
    {
        m_entries[root].right = right;
        if (right >= 0)
        {
            m_entries[right].parent = root;
        }
    }

    IntType insert (IntType root, InsertStruct& is)
    {
        if (root < 0)
        {
            root = getFreeIndex ();
            if (root < 0)
            {
                return -1;
            }

            m_entries[root].left = -1;
            m_entries[root].right = -1;
            m_entries[root].parent = -1;
            m_entries[root].height = 1;
            setValue (root, is.key, is.value);
            is.pos = root;

            ++m_size;
            return root;
        }

        KeyComparator c;

        int r = is.compare (*reinterpret_cast<Key*>(&m_entries[root].key));
        if (r < 0)
        {
            IntType index = insert (m_entries[root].left, is);
            if (index < 0)
            {
                return -1;
            }
            setLeftChild (root, index);
        }
        else if (r > 0)
        {
            IntType index = insert (m_entries[root].right, is);
            if (index < 0)
            {
                return -1;
            }
            setRightChild (root, index);
        }
        else
        {
            updateValue (root, is.value);
            is.pos = root;
            return root;
        }
        return balance (root);
    }

    IntType findMin (IntType root)
    {
        while (m_entries[root].left >= 0)
        {
            root = m_entries[root].left;
        }
        return root;
    }

    IntType remove(IntType root, RemoveStruct& rs)
    {
        if (root < 0)
        {
            return -1;
        }

        int r = rs.compare (*reinterpret_cast<Key*>(&m_entries[root].key));

        if (r < 0)
        {
            setLeftChild (root, remove (m_entries[root].left, rs));
        }
        else if (r > 0)
        {
            setRightChild (root, remove (m_entries[root].right, rs));
        }
        else
        {
            IntType left = m_entries[root].left;
            IntType right = m_entries[root].right;

            rs.pos = root;
            addFreeIndex (root);
            removeKV (root);
            --m_size;

            if (left < 0)
            {
                return right;
            }
            else if (right < 0)
            {
                return left;
            }
            else
            {
                // this is a more complicated case.
                // the solution is the move the smallest of the right branch
                // here.
                IntType min = findMin (right);
                if (min == right)
                {
                    root = right;
                    setLeftChild (root, left);
                }
                else
                {
                    // first detach min from the parent
                    IntType parent = m_entries[min].parent;
                    m_entries[parent].left = -1;
                    if (parent != right)
                    {
                        IntType grandParent = m_entries[parent].parent;
                        parent = balance (parent);
                        setLeftChild (grandParent, parent);
                    }
                    else
                    {
                        right = balance (right);
                    }

                    root = min;
                    m_entries[root].parent = -1;
                    setLeftChild (root, left);
                    setRightChild (root, right);
                }
            }
        }

        return balance (root);
    }

    IntType balance (IntType root)
    {
        IntType left = m_entries[root].left;
        IntType right = m_entries[root].right;
        IntType diff = getHeight (left) - getHeight (right);
        if (diff > 1)
        {
            if (getHeight (m_entries[left].left) >= getHeight (m_entries[left].right))
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
            if (getHeight (m_entries[right].right) >= getHeight (m_entries[right].left))
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

    IntType rotateWithLeftChild (IntType root)
    {
        IntType left = m_entries[root].left;
        setLeftChild (root, m_entries[left].right);
        setRightChild (left, root);
        updateHeight (root);
        updateHeight (left);
        return left;
    }

    IntType rotateWithRightChild (IntType root)
    {
        IntType right = m_entries[root].right;
        setRightChild (root, m_entries[right].left);
        setLeftChild (right, root);
        updateHeight (root);
        updateHeight (right);
        return right;
    }

    IntType doubleWithLeftChild (IntType root)
    {
        setLeftChild (root, rotateWithRightChild (m_entries[root].left));
        return rotateWithLeftChild (root);
    }

    IntType doubleWithRightChild (IntType root)
    {
        setRightChild (root, rotateWithLeftChild (m_entries[root].right));
        return rotateWithRightChild (root);
    }

    /**
     * Search the tree using the key provided.
     *
     * @param   key
     *          the search key
     * @return  the index value of the entry.  -1 if not found.
     */
    IntType search(const Key& key) const
    {
        if (m_size == 0)
        {
            return -1;
        }

        KeyComparator c;
        for (IntType curr = m_root; curr >= 0; )
        {
            int r = c.compare (key, *reinterpret_cast<const Key*>(&m_entries[curr].key));

            if (r < 0)
            {
                curr = m_entries[curr].left;
            }
            else if (r > 0)
            {
                curr = m_entries[curr].right;
            }
            else
            {
                return curr;
            }
        }
        return -1;
    }

    /**
     * Given the index, retrieve the value.
     *
     * @param   index
     *          the entry index.
     * @return  the value at the index.
     */
    Value& getIndexValue (IntType index)
    {
        return *reinterpret_cast<Value*>(&m_values[index]);
    }

    /**
     * Given the index, retrieve the value.
     *
     * @param   index
     *          the entry index.
     * @return  the value at the index.
     */
    const Key& getIndexKey (IntType index)
    {
        return *reinterpret_cast<Key*>(&m_entries[index].key);
    }

    IntType getFreeIndex ()
    {
        if (m_free < 0)
        {
            return -1;
        }
        IntType index = m_free;

        IntType prev = m_entries[index].left;
        if (prev == index)
        {
            m_free = -1;
        }
        else
        {
            m_free = m_entries[index].right;
            m_entries[prev].right = m_free;
            m_entries[m_free].left = prev;
        }
        return index;
    }

    void addFreeIndex(IntType index)
    {
        if (m_free < 0)
        {
            m_entries[index].left = index;
            m_entries[index].right = index;
            m_free = index;
        }
        else
        {
            IntType prev = m_entries[m_free].left;
            m_entries[index].left = prev;
            m_entries[index].right = m_free;
            m_entries[prev].right = index;
            m_entries[m_free].left = index;
            m_free = index;
        }
    }

    void printNode (IntType root)
    {
        if (root < 0)
            return;
        std::cout << (int)root << " -- {";
        if (m_entries[root].left >= 0)
        {
            std::cout << " " << (int)m_entries[root].left;
        }
        if (m_entries[root].right >= 0)
        {
            std::cout << " " << (int)m_entries[root].right;
        }
        std::cout << " }" << std::endl;
        std::cout << (int)root << " [ label=";
        std::cout << (*reinterpret_cast<Value*>(&m_values[root]));
        std::cout << " ]" << std::endl;
        printNode (m_entries[root].left);
        printNode (m_entries[root].right);
    }

private:
    // head entry.  -1 to indicate none.
    IntType     m_root;
    // first free entry
    IntType     m_free;
    // size of the LRU.
    IntType     m_size;
    // node entries
    Node        m_entries[TREESIZE];
    ValueType   m_values[TREESIZE];
};

}   // namespace cookmem

#endif  // COOK_AVLTREE_H
