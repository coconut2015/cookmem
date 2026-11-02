/*
 * Copyright (c) 2021-2026 Heng Yuan
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
#ifndef COOK_MALLOC_MEM_ARENA_H
#define COOK_MALLOC_MEM_ARENA_H

#include <cstdlib>

#include "arena.h"
#include "config.h"

namespace cookmem
{

class SegmentStatistics
{
public:
    typedef std::size_t size_type;

public:
    SegmentStatistics ()
    : m_currentSegments (0),
      m_maxSegments (0),
      m_currentSize (0),
      m_maxSize (0)
    {
    }

    void
    print () const
    {
        printf ("current segments: %zu, current size: %zu, max segments: %zu, max size: %zu\n",
                m_currentSegments,
                m_currentSize,
                m_maxSegments,
                m_maxSize);
    }

    void
    newSegment (size_type segSize)
    {
        ++m_currentSegments;
        m_currentSize += segSize;

        if (m_maxSegments < m_currentSegments)
        {
            m_maxSegments = m_currentSegments;
        }
        if (m_maxSize < m_currentSize)
        {
            m_maxSize = m_currentSize;
        }
    }

    void
    freeSegment (size_type segSize)
    {
        --m_currentSegments;
        m_currentSize -= segSize;
    }

private:
    size_type   m_currentSegments;
    size_type   m_maxSegments;
    size_type   m_currentSize;
    size_type   m_maxSize;
};

/**
 * An malloc/free based memory arena.
 */
class MallocArena
{
public:
    /**
     * Constructor.
     *
     * @param   minSize
     *          minimum segment size.  It should be noted that this value
     *          needs to be a multiple of 16.
     */
    MallocArena (std::size_t minSize = DEFAULT_MIN_SEGMENT_SIZE)
    : m_minSize (minSize),
      m_stats ()
    {
    }

    /**
     * Allocate an arena segment using malloc.
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
        m_stats.newSegment (size);
        return malloc (size);
    }

    /**
     * Free an arena segment using free.
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
        m_stats.freeSegment (size);

        free (ptr);
        return false;
    }

    void
    printStats ()
    {
        m_stats.print ();
    }

private:
    std::size_t         m_minSize;
    SegmentStatistics   m_stats;
};

}   // namespace cookmem

#endif  // COOK_MALLOC_MEM_ARENA_H
