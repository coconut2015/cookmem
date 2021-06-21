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
#ifndef COOK_MEM_ARENA_H
#define COOK_MEM_ARENA_H

#include <cstddef>
#include <stdint.h>

#include <cookptravltree.h>

namespace cookmem
{

/**
 * An arena is used to retrieve a page of memory.
 *
 * This class here is merely to define the functions needed to implement a MemArena.
 */
class MemArena
{
public:
    virtual ~MemArena() {}

    /**
     * Allocate an arena segment.
     *
     * @param [in,out]  size
     *          the size of the page.  This value is updated upon successful
     *          request to indicate the actual size obtained.
     * @return  the allocated pointer.  nullptr is allocation failed.
     */
    virtual void* getSegment(std::size_t& size) = 0;

    /**
     * Free an arena segment.
     *
     * @param   ptr
     *          the pointer to be freed.
     * @param   size
     *          the size of the pointer.
     * @return  true if there is an error.  false is okay.
     */
    virtual bool freeSegment(void* ptr, std::size_t size) = 0;
};

/**
 * This memory arena simply wraps around a piece of memory which is used for
 * memory allocation.
 */
class FixedArena
{
public:
    /**
     * Arena constructor.
     *
     * @param   ptr
     *          the memory buffer for memory allocation.
     * @param   size
     *          the size of the memory pointer.
     */
    FixedArena(void* ptr, std::size_t size)
    : m_page (), m_size (), m_used (false)
    {
        std::size_t remain = (std::size_t)ptr & 0xfUL;
        if (remain)
        {
            std::size_t inc = (0x10 - remain);
            m_page = (void*)((char*)ptr + inc);
            size -= inc;
        }
        else
        {
            m_page = ptr;
        }
        size &= 0xfffffffffffffff0UL;
        m_size = size;
    }

    /**
     * Allocate an arena segment.
     *
     * @param [in,out]  size
     *          the size of the page.  This value is updated upon successful
     *          request to indicate the actual size obtained.
     * @return  the allocated pointer.  nullptr if allocation failed.
     */
    void* getSegment(std::size_t& size)
    {
        if (m_used || size > m_size)
        {
            return NULL;
        }
        m_used = true;
        size = m_size;
        return m_page;
    }

    /**
     * Free an arena segment.
     *
     * @param   ptr
     *          the pointer to be freed.
     * @param   size
     *          the size of the pointer.
     * @return  true if there is an error.  false is okay.
     */
    bool freeSegment(void* ptr, std::size_t size)
    {
        if (!m_used || ptr != m_page || size != m_size)
        {
            return true;
        }
        m_used = false;
        return false;
    }

private:
    void*       m_page;
    std::size_t m_size;
    bool        m_used;
};

/**
 * CachedArena is used to cache the segments released and see if they can be
 * reused in the future segment request.
 */
template<class Arena>
class CachedArena
{
public:
    /**
     * Constructor
     *
     * @param   arena
     *          the memory arena that does the actual memory allocation
     */
    CachedArena (Arena& arena)
    : m_arena (arena),
      m_tree ()
    {
    }

    /**
     * Allocate an arena segment.
     *
     * It first checks if there is a cached segment that could satisfy the request.
     * If not, it calls the actual memory arena to allocate the segment.
     *
     * @param [in,out]  size
     *          the size of the page.  This value is updated upon successful
     *          request to indicate the actual size obtained.
     * @return  the allocated pointer.  nullptr is allocation failed.
     */
    void* getSegment(std::size_t& size)
    {
        void* seg = m_tree.remove(size);
        if (seg)
        {
            return seg;
        }
        return m_arena.getSegment(size);
    }

    /**
     * Free an arena segment.
     *
     * The segment release is cached for the next get attempt that
     * can be satisfied.
     *
     * @param   ptr
     *          the pointer to be freed.
     * @param   size
     *          the size of the pointer.
     * @return  false.
     */
    bool freeSegment(void* ptr, std::size_t size)
    {
        m_tree.add(ptr, size);
        return false;
    }

private:
    Arena&      m_arena;
    PtrAVLTree  m_tree;
};


}   // namespace cookmem

#endif  // COOK_MEM_ARENA_H
