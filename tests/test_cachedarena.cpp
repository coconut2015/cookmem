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
#include <iostream>

#include <cookmem.h>
#include "unittest.h"

#define NUM_ENTRIES 9

/**
 * An arena that counts the segments it has handed out and not yet freed.
 */
class CountingArena
{
public:
    CountingArena ()
    : m_numSegments (0)
    {
    }

    void*
    getSegment (std::size_t& size)
    {
        void* ptr = m_arena.getSegment (size);
        if (ptr != nullptr)
        {
            ++m_numSegments;
            ++s_totalSegments;
        }
        return ptr;
    }

    bool
    freeSegment (void* ptr, std::size_t size)
    {
        --m_numSegments;
        --s_totalSegments;
        return m_arena.freeSegment (ptr, size);
    }

    void
    releaseAll ()
    {
    }

    int
    getNumSegments () const
    {
        return m_numSegments;
    }

    /** The number of live segments across all CountingArena instances. */
    static int
    getTotalSegments ()
    {
        return s_totalSegments;
    }

private:
    static int              s_totalSegments;

    cookmem::MallocArena    m_arena;
    int                     m_numSegments;
};

int CountingArena::s_totalSegments = 0;

static int
test1 ()
{
    cookmem::MallocArena arena;
    cookmem::CachedArena<cookmem::MallocArena> cachedArena (arena);
    cookmem::NoActionMemLogger logger;

    {
        cookmem::MemContext<cookmem::CachedArena<cookmem::MallocArena>, cookmem::NoActionMemLogger> memCtx (cachedArena, logger);

        void* ptrs[NUM_ENTRIES];

        std::size_t size = 3;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            size *= 10;
            ptrs[i] = memCtx.allocate (size);
            ASSERT_EQ (true, memCtx.contains (ptrs[i]));
            ASSERT_NE (nullptr, ptrs[i]);
        }

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            memCtx.deallocate (ptrs[i]);
        }

        memCtx.releaseAll ();
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_EQ (false, memCtx.contains (ptrs[i]));
        }
    }

    {
        cookmem::MemContext<cookmem::CachedArena<cookmem::MallocArena>, cookmem::NoActionMemLogger> memCtx(cachedArena, logger);

        void* ptrs[NUM_ENTRIES];

        std::size_t size = 3;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            size *= 10;
            ptrs[i] = memCtx.allocate (size);
            ASSERT_EQ (true, memCtx.contains (ptrs[i]));
            ASSERT_NE (nullptr, ptrs[i]);
        }

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            memCtx.deallocate (ptrs[i]);
        }

        memCtx.releaseAll();
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_EQ (false, memCtx.contains (ptrs[i]));
        }
    }

    return 0;
}


/**
 * CachedArena::releaseAll () and the destructor return the cached segments
 * to the underlying arena.
 */
template<class TreeType>
static int
testReleaseAll ()
{
    CountingArena arena;
    {
        cookmem::CachedArena<CountingArena, TreeType> cachedArena (arena);

        void* ptrs[NUM_ENTRIES];
        std::size_t sizes[NUM_ENTRIES];
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            // Two segments of each size, to fill the lists of equal sizes.
            sizes[i] = 4096 * (1 + i / 2);
            ptrs[i] = cachedArena.getSegment (sizes[i]);
            ASSERT_NE (nullptr, ptrs[i]);
        }
        ASSERT_EQ (NUM_ENTRIES, arena.getNumSegments ());

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            cachedArena.freeSegment (ptrs[i], sizes[i]);
        }
        // Freed segments are cached, not returned.
        ASSERT_EQ (NUM_ENTRIES, arena.getNumSegments ());

        cachedArena.releaseAll ();
        ASSERT_EQ (0, arena.getNumSegments ());

        // The arena is still usable after releaseAll ().
        std::size_t size = 4096;
        void* ptr = cachedArena.getSegment (size);
        ASSERT_NE (nullptr, ptr);
        cachedArena.freeSegment (ptr, size);
        ASSERT_EQ (1, arena.getNumSegments ());
    }
    // The destructor returns the remaining cached segment.
    ASSERT_EQ (0, arena.getNumSegments ());
    return 0;
}

/**
 * Destroying a CachedMemContext returns all of its segments to the
 * underlying arena, which the context owns.
 */
static int
testCachedMemContext ()
{
    const int numSegments = CountingArena::getTotalSegments ();
    {
        cookmem::CachedMemContext<CountingArena> memCtx;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_NE (nullptr, memCtx.allocate (1 << 20));
        }
        ASSERT_EQ (true, numSegments < CountingArena::getTotalSegments ());

        // releaseAll () on the context only moves the segments to the cache.
        memCtx.releaseAll ();
        ASSERT_EQ (true, numSegments < CountingArena::getTotalSegments ());

        // releaseAll () on the arena returns them.
        memCtx.getArena ().releaseAll ();
        ASSERT_EQ (numSegments, CountingArena::getTotalSegments ());

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_NE (nullptr, memCtx.allocate (1 << 20));
        }
        ASSERT_EQ (true, numSegments < CountingArena::getTotalSegments ());

        // releaseAllToSystem () does both.
        memCtx.releaseAllToSystem ();
        ASSERT_EQ (numSegments, CountingArena::getTotalSegments ());

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_NE (nullptr, memCtx.allocate (1 << 20));
        }
    }
    // Destroying the context returns the rest.
    ASSERT_EQ (numSegments, CountingArena::getTotalSegments ());
    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, test1 ());
        ASSERT_EQ (0, testReleaseAll<cookmem::PtrRBTree> ());
        ASSERT_EQ (0, testReleaseAll<cookmem::PtrAVLTree> ());
        ASSERT_EQ (0, testCachedMemContext ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }

    return 0;
}
