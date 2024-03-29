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
#ifndef COOK_MMAP_MEM_ARENA_H
#define COOK_MMAP_MEM_ARENA_H

#ifdef WIN32
#include <windows.h>
#else   // WIN32
#include <sys/mman.h>
#endif  // WIN32

#include "cookmemarena.h"

namespace cookmem
{

#ifdef WIN32
/**
 * An mmap based memory arena adapted for Windows.
 */
class MmapArena
{
public:
    /**
     * Constructor.
     *
     * @param   minSize
     *          minimum segment size.  It should be noted that this value
     *          needs to be a multiple of 16.
     * @param   prot
     *          protection flag
     * @param   type
     *          allocation type
     */
    MmapArena (std::size_t minSize = 65536, DWORD prot = PAGE_READWRITE, DWORD type = ( MEM_RESERVE | MEM_COMMIT ))
      : m_minSize (minSize),
        m_prot (prot),
        m_type (type)
    {
    }

    /**
     * Allocate an arena segment using VirtualAlloc().
     *
     * @param   size
     *          the size of the segment.  This value is updated upon successful
     *          request to indicate the actual size obtained.
     * @return  the allocated pointer.  nullptr is allocation failed.
     */
    void*
    getSegment(std::size_t& size)
    {
        if (size < m_minSize)
        {
            size = m_minSize;
        }
        return VirtualAlloc (nullptr, size, m_type, m_prot);
    }

    /**
     * Free an arena segment using munamp().
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
        return VirtualFree (ptr, 0, MEM_RELEASE) == 0;
    }

private:
    std::size_t m_minSize;
    DWORD       m_prot;
    DWORD       m_type;
};
#else   // WIN32
/**
 * An mmap based memory arena.
 */
class MmapArena
{
public:
    /**
     * Constructor.
     *
     * @param   minSize
     *          minimum segment size.  It should be noted that this value
     *          needs to be a multiple of 16.
     * @param   prot
     *          mmap protection flag
     * @param   flag
     *          mmap flag
     */
    MmapArena(std::size_t minSize = 65536, int prot = ( PROT_READ | PROT_WRITE ), int flag = ( MAP_PRIVATE | MAP_ANONYMOUS ))
      : m_minSize (minSize),
        m_prot (prot),
        m_flag (flag)
    {
    }

    /**
     * Allocate an arena segment using mmap().
     *
     * @param   size
     *          the size of the segment.  This value is updated upon successful
     *          request to indicate the actual size obtained.
     * @return  the allocated pointer.  nullptr is allocation failed.
     */
    void*
    getSegment (std::size_t& size)
    {
        if (size < m_minSize)
        {
            size = m_minSize;
        }
        void* ptr = mmap(nullptr, size, m_prot, m_flag, -1, 0);
        if (ptr == MAP_FAILED)
        {
            return nullptr;
        }
        return ptr;
    }

    /**
     * Free an arena segment using munamp().
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
        return munmap (ptr, size) != 0;
    }

private:
    std::size_t m_minSize;
    int         m_prot;
    int         m_flag;
};
#endif  // WIN32

}   // namespace cookmem

#endif  // COOK_MMAP_MEM_ARENA_H
