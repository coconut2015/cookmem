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
#ifndef COOK_PTR_CIRCULAR_LIST_H
#define COOK_PTR_CIRCULAR_LIST_H

#include <cookutils.h>

namespace cookmem
{

#undef COOKASSERT
#define COOKASSERT(b) do { if (!(b)) throw cookmem::MemException("Assertion failure."); } while (0)

/**
 * A circular list maintenance logic.
 *
 * The Node class should have next and prev variables.
 */
template<class Node>
class CircularList
{
public:
    CircularList () : m_head (nullptr) {}

    /**
     * Add a node to the list.
     *
     * @param   ptr
     *          a node pointer to be added to the list
     */
    void add (Node* ptr)
    {
        if (m_head)
        {
            Node* curr = m_head;
            Node* next = m_head->next;
            curr->next = ptr;
            next->prev = ptr;
            ptr->next = next;
            ptr->prev = curr;
        }
        else
        {
            ptr->next = ptr;
            ptr->prev = ptr;
            m_head = ptr;
        }
    }

    /**
     * Remove the first node from the list.
     *
     * @return  the first node in the list.
     */
    Node* remove ()
    {
        if (m_head == nullptr)
        {
            return nullptr;
        }

        Node* prev = m_head->prev;
        Node* next = m_head->next;
        Node* head = m_head;

        if (m_head == prev)
        {
            COOKASSERT(m_head == next);
            m_head = nullptr;
        }
        else
        {
            prev->next = next;
            next->prev = prev;
            m_head = next;
        }

        return head;
    }

    /**
     *
     * @param   chunk
     *          the chunk to be removed.
     * @return  true if the list is now empty.
     *          false otherwise.
     */
    bool remove (Node* chunk)
    {
        Node* prev = chunk->prev;
        Node* next = chunk->next;

        if (prev == chunk)
        {
            COOKASSERT(m_head == chunk && next == chunk);
            m_head = nullptr;
            return true;
        }
        else
        {
            if (m_head == chunk)
            {
                m_head = next;
            }
            prev->next = next;
            next->prev = prev;
            return false;
        }
    }

    /**
     * Check if the list is empty.
     *
     * @return  true if the list is empty.
     *          false otherwise.
     */
    bool isEmpty () const { return m_head == nullptr; }
private:
    /** The first node in the list */
    Node*   m_head;
};
#undef COOKASSERT

}   // namespace cookmem

#endif  // COOK_PTR_CIRCULAR_LIST_H
