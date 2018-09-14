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
#ifndef COOK_LRU_H
#define COOK_LRU_H

#include <new>
#include <cstddef>
#include <cstring>
#include <stdint.h>

#include <cookavltree.h>

namespace cookmem
{

/**
 * A simple fixed LRU.
 *
 * @param   Key
 *          the key type
 * @param   KeyComparator
 *          the key comparison type
 * @param   Value
 *          the value type
 * @param   LRUSIZE
 *          the number of entries in the LRU.
 */
template<class Key, class KeyComparator, class Value, std::size_t LRUSIZE>
class FixedLRU
{
private:
    typedef FixedAVLTree<Key, KeyComparator, Value, LRUSIZE>   TreeType;

public:
    /** The integer type */
    typedef typename TreeType::IntType  IntType;

private:
    struct Entry
    {
        IntType     prev;
        IntType     next;
    };

public:
    /**
     * Constructor.
     */
    FixedLRU()
    : m_head(-1), m_entries()
    {
    }
    /**
     * Destructor.
     */
    ~FixedLRU ()
    {
        clear ();
    }

    /**
     * Clear the LRU.
     */
    void clear()
    {
        m_tree.clear ();
        m_head = -1;
    }

    /**
     * Add a key / value pair to the search tree.
     *
     * @param   key
     *          the key to be inserted / updated.
     * @param   value
     *          the value to be inserted / updated.
     * @return  the inserting / updating position.
     */
    IntType add(const Key& key, const Value& value)
    {
        std::size_t oldSize = m_tree.size();
        IntType index = m_tree.add(key, value);
        if (index < 0)
        {
            // We do not have a free entry, thus, we need to pop a last used
            // one from the tail, which is the previous item of the head.
            //
            // However, since we are going to add this prev item as the new
            // head, the overall operation is simply setting the head to the
            // prev.
            IntType prev = m_entries[m_head].prev;
            m_tree.remove(m_tree.getIndexKey (prev));
            m_head = prev;
            return m_tree.add(key, value);
        }
        else
        {
            std::size_t newSize = m_tree.size();
            if (newSize > oldSize)
            {
                // a new entry added.
                addIndex (index);
            }
            else
            {
                if (index != m_head)
                {
                    removeIndex (index);
                    addIndex(index);
                }
            }
            return index;
        }
    }

    /**
     * Search the LRU given the key.
     * @param   key
     *          the search key
     * @return  the value found.  nullptr is not found.
     */
    Value* get(const Key& key)
    {
        IntType index = m_tree.search(key);
        if (index < 0)
        {
            return NULL;
        }

        if (index != m_head)
        {
            removeIndex(index);
            addIndex(index);
        }

        return &m_tree.getIndexValue(index);
    }

    /**
     * Remove a particular entry given the search key.
     * @param   key
     *          the search key
     * @return  true if the entry is removed.  false if it is not found.
     */
    bool remove(const Key& key)
    {
        IntType index = m_tree.remove(key);
        if (index < 0)
        {
            return false;
        }

        removeIndex(index);
        return true;
    }

    /**
     * Gets the current size of the LRU.
     *
     * @return  the current size of the LRU.
     */
    std::size_t size() const
    {
        return m_tree.size ();
    }

    /**
     * A debugging function to print the LRU entries.
     */
    void printLRU ()
    {
        if (m_head < 0)
        {
            std::cout << "empty" << std::endl;
        }
        else
        {
            IntType curr = m_head;

            do
            {
                std::cout << m_tree.getIndexKey (curr) << " ";
                curr = m_entries[curr].next;
            }
            while (curr != m_head);
            std::cout << std::endl;
        }
    }
private:
    void removeIndex(IntType index)
    {
        // check if the index is at the tail
        IntType next = m_entries[index].next;
        if (next == index)
        {
            m_head = -1;
            return;
        }

        IntType prev = m_entries[index].prev;
        m_entries[prev].next = next;
        m_entries[next].prev = prev;

        if (m_head == index)
        {
            m_head = next;
        }
    }

    void addIndex(IntType index)
    {
        if (m_head < 0)
        {
            m_entries[index].prev = index;
            m_entries[index].next = index;
        }
        else
        {
            IntType prev = m_entries[m_head].prev;
            m_entries[index].prev = prev;
            m_entries[index].next = m_head;
            m_entries[prev].next = index;
            m_entries[m_head].prev = index;
        }
        m_head = index;
    }

private:
    /** avl search tree */
    TreeType    m_tree;
    /** LRU head entry.  -1 to indicate none. */
    IntType     m_head;
    /** LRU entries */
    Entry       m_entries[LRUSIZE];
};

}   // namespace cookmem

#endif  // COOK_LRU_H
