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
#ifndef COOK_MEM_ARENA_H
#define COOK_MEM_ARENA_H

#include <cstddef>

#include "rbtree.h"

namespace cookmem
{

/**
 * An arena is used to retrieve a page of memory.
 *
 * This class here is merely to define the interface needed to implement a MemArena.
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
    virtual void*
    getSegment (std::size_t& size) = 0;

    /**
     * Free an arena segment.
     *
     * @param   ptr
     *          the pointer to be freed.
     * @param   size
     *          the size of the pointer.
     * @return  true if there is an error.  false is okay.
     */
    virtual bool
    freeSegment (void* ptr, std::size_t size) = 0;

    /**
     * Release all segments held by this arena.  Segments that a memory pool
     * is still using are not affected.  An arena that keeps no segments
     * does nothing.
     */
    virtual void
    releaseAll () = 0;
};

/**
 * This memory arena simply wraps around a piece of memory which is used for
 * memory allocation.
 *
 * This class implements the interface of MemArena without explicitly
 * inheriting from it.  This is to avoid the overhead of virtual function
 * calls and allows inlining of the functions.
 */
class FixedArena
{
public:
    /**
     * Constructor.
     *
     * @param   ptr
     *          the memory buffer for memory allocation.
     *          If it is not 16-byte aligned, the start address is advanced
     *          to the next 16-byte boundary.
     * @param   size
     *          The size of the memory buffer. The usable size is reduced by
     *          any bytes skipped to align the start address, then rounded
     *          down to a multiple of 16.
     */
    FixedArena (void* ptr, std::size_t size)
    : m_used (false)
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
    void*
    getSegment (std::size_t& size)
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
    bool
    freeSegment (void* ptr, std::size_t size)
    {
        if (!m_used || ptr != m_page || size != m_size)
        {
            return true;
        }
        m_used = false;
        return false;
    }

    /**
     * Release all segments held by this arena.  The buffer belongs to the
     * caller, so this function does nothing.
     */
    void
    releaseAll ()
    {
    }

private:
    void*       m_page;
    std::size_t m_size;
    bool        m_used;
};

/**
 * CachedArena is used to cache the segments released and see if they can be
 * reused in the future segment request.
 *
 * This class implements the interface of MemArena without explicitly
 * inheriting from it.  This is to avoid the overhead of virtual function
 * calls and allows inlining of the functions.
 *
 * @tparam  Arena
 *          the memory arena that does the actual memory allocation
 * @tparam  TreeType
 *          The type of the tree used to cache the released segments.
 *          The default is PtrRBTree.
 */
template<class Arena, class TreeType = PtrRBTree>
class CachedArena
{
public:
    /** The type of the tree used to cache the released segments. */
    typedef TreeType   PtrTreeType;
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
     * Destructor.  The cached segments are returned to the underlying arena.
     */
    ~CachedArena ()
    {
        releaseAll ();
    }

    CachedArena (const CachedArena&) = delete;
    CachedArena& operator= (const CachedArena&) = delete;

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
    void*
    getSegment (std::size_t& size)
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
    bool
    freeSegment (void* ptr, std::size_t size)
    {
        m_tree.add (ptr, size);
        return false;
    }

    /**
     * Return all the cached segments to the underlying arena.  Segments
     * that a memory pool is still using are not affected.
     */
    void
    releaseAll ()
    {
        for (;;)
        {
            std::size_t size = 0;
            void* seg = m_tree.remove (size);
            if (seg == nullptr)
            {
                break;
            }
            m_arena.freeSegment (seg, size);
        }
    }

private:
    Arena&      m_arena;
    PtrTreeType m_tree;
};


}   // namespace cookmem

#endif  // COOK_MEM_ARENA_H
