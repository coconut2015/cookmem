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
// Tests for growing in place on reallocate, preferring the designated victim
// when it fits better than the tree bins, and releasing large segments as
// soon as they are freed.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <cookmem.h>
#include "unittest.h"

/**
 * A context whose arena returns segments of exactly the requested size, so
 * that every new segment holds one allocation and leaves no free remainder.
 */
template<class TreeType = cookmem::PtrRBTree>
struct ExactContext
{
    cookmem::MmapArena          arena;
    cookmem::NoActionMemLogger  logger;
    cookmem::MemContext<cookmem::MmapArena, cookmem::NoActionMemLogger, TreeType> memCtx;

    ExactContext (std::uint8_t paddingSize = 0)
    : arena (0),
      logger (),
      memCtx (arena, logger, paddingSize)
    {
        memCtx.setTrimEmptySegmentsInterval (~static_cast<std::size_t> (0));
    }
};

static bool
isFilled (const void* ptr, std::size_t size, unsigned char value)
{
    const unsigned char* p = static_cast<const unsigned char*> (ptr);
    for (std::size_t i = 0; i < size; ++i)
    {
        if (p[i] != value)
        {
            return false;
        }
    }
    return true;
}

/**
 * reallocate () grows into a free next chunk without moving.
 */
template<class TreeType>
static int
testGrowInPlace (std::uint8_t paddingSize)
{
    cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::NoActionMemLogger, TreeType> memCtx (paddingSize);
    memCtx.setStoringExactSize (true);

    // Growing into a free chunk in a bin, leaving a remainder.
    void* a = memCtx.allocate (1000);
    void* b = memCtx.allocate (1000);
    void* c = memCtx.allocate (100);
    ASSERT_NE (nullptr, c);
    std::memset (a, 0x5a, 1000);
    memCtx.deallocate (b);

    void* grown = memCtx.reallocate (a, 1500);
    ASSERT_EQ (a, grown);
    ASSERT_EQ (1500u, memCtx.getSize (a));
    ASSERT_EQ (true, isFilled (a, 1000, 0x5a));
    std::memset (a, 0x6b, 1500);
    memCtx.validate ();

    // Growing as far as c, which is in use, allows.
    std::size_t limit = reinterpret_cast<char*> (c) - reinterpret_cast<char*> (a) -
                        2 * sizeof (std::size_t) - paddingSize - (paddingSize ? 1 : 0);
    grown = memCtx.reallocate (a, limit);
    ASSERT_EQ (a, grown);
    ASSERT_EQ (limit, memCtx.getSize (a));
    ASSERT_EQ (true, isFilled (a, 1500, 0x6b));
    memCtx.validate ();

    // One more byte does not fit, so the memory moves.
    grown = memCtx.reallocate (a, limit + 1);
    ASSERT_NE (nullptr, grown);
    ASSERT_NE (a, grown);
    ASSERT_EQ (true, isFilled (grown, 1500, 0x6b));
    memCtx.validate ();

    // deallocate () checks the padding of every chunk.
    memCtx.deallocate (grown);
    memCtx.deallocate (c);
    memCtx.validate ();

    // Growing into the designated victim.  In a new context, the first
    // allocation is followed by the free rest of its segment, which is the
    // designated victim.
    cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::NoActionMemLogger, TreeType> newCtx (paddingSize);
    void* f = newCtx.allocate (300);
    ASSERT_NE (nullptr, f);
    std::memset (f, 0x7c, 300);
    void* grownF = newCtx.reallocate (f, 10000);
    ASSERT_EQ (f, grownF);
    ASSERT_EQ (true, isFilled (f, 300, 0x7c));
    newCtx.validate ();
    newCtx.deallocate (f);
    newCtx.validate ();
    return 0;
}

/**
 * Random reallocations keep the data and the pool consistent.
 */
static int
testRandomReallocate ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx (4);

    struct Block
    {
        unsigned char*  ptr;
        std::size_t     size;
        unsigned char   value;
    };
    std::vector<Block> blocks (50);
    for (std::size_t i = 0; i < blocks.size (); ++i)
    {
        blocks[i].size = 0;
        blocks[i].ptr = nullptr;
        blocks[i].value = static_cast<unsigned char> (i + 1);
    }

    std::uint32_t seed = 4321;
    for (int i = 0; i < 20000; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        Block& block = blocks[(seed >> 8) % blocks.size ()];
        seed = seed * 1664525u + 1013904223u;
        std::size_t newSize = (seed >> 8) % ((seed & 1) ? 400 : 30000);

        ASSERT_EQ (true, block.ptr == nullptr || isFilled (block.ptr, block.size, block.value));
        unsigned char* ptr = static_cast<unsigned char*> (memCtx.reallocate (block.ptr, newSize));
        ASSERT_NE (nullptr, ptr);
        std::size_t kept = block.size < newSize ? block.size : newSize;
        ASSERT_EQ (true, isFilled (ptr, kept, block.value));
        std::memset (ptr, block.value, newSize);
        block.ptr = ptr;
        block.size = newSize;

        if ((i % 1000) == 0)
        {
            memCtx.validate ();
        }
    }

    for (Block& block : blocks)
    {
        memCtx.deallocate (block.ptr);
    }
    memCtx.validate ();
    return 0;
}

/**
 * For a large request, the designated victim is used when it fits better
 * than the best tree chunk, and the tree chunk otherwise.
 */
template<class TreeType>
static int
testPreferBetterFit ()
{
    {
        ExactContext<TreeType> ctx;
        void* bigPtr = ctx.memCtx.allocate (20000);
        void* smallPtr = ctx.memCtx.allocate (5000);
        ASSERT_NE (nullptr, smallPtr);

        // small becomes the designated victim, and big goes to a tree.
        ctx.memCtx.deallocate (smallPtr);
        ctx.memCtx.deallocate (bigPtr);
        ctx.memCtx.validate ();

        // Both fit, and the designated victim fits better.
        ASSERT_EQ (smallPtr, ctx.memCtx.allocate (4000));
        // Only the tree chunk fits.
        ASSERT_EQ (bigPtr, ctx.memCtx.allocate (6000));
        ctx.memCtx.validate ();
    }
    {
        ExactContext<TreeType> ctx;
        void* bigPtr = ctx.memCtx.allocate (20000);
        void* smallPtr = ctx.memCtx.allocate (5000);
        ASSERT_NE (nullptr, smallPtr);

        // big becomes the designated victim, and small goes to a tree.
        ctx.memCtx.deallocate (bigPtr);
        ctx.memCtx.deallocate (smallPtr);
        ctx.memCtx.validate ();

        // Both fit, and the tree chunk fits better.
        ASSERT_EQ (smallPtr, ctx.memCtx.allocate (4000));
        ASSERT_EQ (bigPtr, ctx.memCtx.allocate (6000));
        ctx.memCtx.validate ();
    }
    return 0;
}

/**
 * A large allocation in a segment of its own releases the segment as soon
 * as it is freed.
 */
static int
testImmediateRelease ()
{
    // The default threshold.
    {
        cookmem::SimpleMemContext<cookmem::MmapArena> memCtx;
        ASSERT_EQ (cookmem::DEFAULT_IMMEDIATE_RELEASE_THRESHOLD, memCtx.getImmediateReleaseThreshold ());

        void* smallPtr = memCtx.allocate (100);
        ASSERT_NE (nullptr, smallPtr);
        std::size_t footprint = memCtx.getFootprint ();

        void* bigPtr = memCtx.allocate (1024 * 1024);
        ASSERT_NE (nullptr, bigPtr);
        ASSERT_EQ (true, memCtx.getFootprint () > footprint);
        memCtx.deallocate (bigPtr);
        ASSERT_EQ (footprint, memCtx.getFootprint ());
        ASSERT_EQ (false, memCtx.contains (bigPtr));
        memCtx.validate ();

        // A smaller allocation keeps its segment until the trimming pass.
        void* medium = memCtx.allocate (100000);
        ASSERT_NE (nullptr, medium);
        std::size_t mediumFootprint = memCtx.getFootprint ();
        ASSERT_EQ (true, mediumFootprint > footprint);
        memCtx.deallocate (medium);
        ASSERT_EQ (mediumFootprint, memCtx.getFootprint ());

        // A lower threshold releases it.
        memCtx.setImmediateReleaseThreshold (50000);
        medium = memCtx.allocate (100000);
        ASSERT_EQ (mediumFootprint, memCtx.getFootprint ());
        memCtx.deallocate (medium);
        ASSERT_EQ (footprint, memCtx.getFootprint ());
        memCtx.deallocate (smallPtr);
        memCtx.validate ();
    }

    // 0 disables the immediate release.
    {
        cookmem::SimpleMemContext<cookmem::MmapArena> memCtx;
        memCtx.setImmediateReleaseThreshold (0);
        ASSERT_EQ (0u, memCtx.getImmediateReleaseThreshold ());

        void* bigPtr = memCtx.allocate (1024 * 1024);
        std::size_t footprint = memCtx.getFootprint ();
        memCtx.deallocate (bigPtr);
        ASSERT_EQ (footprint, memCtx.getFootprint ());
        memCtx.validate ();
    }

    // A segment shared with other allocations is kept until all of them are
    // freed, and a small allocation does not release it either.
    {
        cookmem::MmapArena arena (4 * 1024 * 1024);
        cookmem::NoActionMemLogger logger;
        cookmem::MemContext<cookmem::MmapArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

        void* bigPtr = memCtx.allocate (1024 * 1024);
        void* smallPtr = memCtx.allocate (100);
        ASSERT_NE (nullptr, smallPtr);
        std::size_t footprint = memCtx.getFootprint ();

        memCtx.deallocate (bigPtr);
        ASSERT_EQ (footprint, memCtx.getFootprint ());
        memCtx.deallocate (smallPtr);
        ASSERT_EQ (footprint, memCtx.getFootprint ());
        memCtx.validate ();

        // Freed last, a large allocation releases the shared segment, after
        // merging with the free chunks on both sides.
        smallPtr = memCtx.allocate (300000);
        bigPtr = memCtx.allocate (1024 * 1024);
        ASSERT_EQ (footprint, memCtx.getFootprint ());
        memCtx.deallocate (smallPtr);
        memCtx.deallocate (bigPtr);
        ASSERT_EQ (0u, memCtx.getFootprint ());
        memCtx.validate ();
    }

    // A large allocation that fills the rest of a segment is merged into the
    // free chunk before it in place, then releases the segment.
    {
        static char buffer[2 * 1024 * 1024];
        cookmem::FixedArena arena (buffer, sizeof (buffer));
        cookmem::NoActionMemLogger logger;
        cookmem::MemContext<cookmem::FixedArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

        void* first = memCtx.allocate (1000);
        ASSERT_NE (nullptr, first);

        // Find the largest allocation that fits in the rest of the buffer.
        std::size_t low = 0;
        std::size_t high = sizeof (buffer);
        while (low + 1 < high)
        {
            std::size_t mid = low + (high - low) / 2;
            void* ptr = memCtx.allocate (mid);
            if (ptr != nullptr)
            {
                memCtx.deallocate (ptr);
                low = mid;
            }
            else
            {
                high = mid;
            }
        }
        void* rest = memCtx.allocate (low);
        ASSERT_NE (nullptr, rest);
        ASSERT_EQ (nullptr, memCtx.allocate (1));

        memCtx.deallocate (first);
        memCtx.deallocate (rest);
        ASSERT_EQ (0u, memCtx.getFootprint ());
        memCtx.validate ();

        // The buffer can be used again.
        void* again = memCtx.allocate (1024 * 1024);
        ASSERT_NE (nullptr, again);
        memCtx.deallocate (again);
    }

    // CachedArena keeps the released segment for reuse.
    {
        cookmem::CachedMemContext<> memCtx;
        void* bigPtr = memCtx.allocate (1024 * 1024);
        ASSERT_NE (nullptr, bigPtr);
        memCtx.deallocate (bigPtr);
        ASSERT_EQ (0u, memCtx.getFootprint ());
        ASSERT_EQ (bigPtr, memCtx.allocate (1024 * 1024));
    }

    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, testGrowInPlace<cookmem::PtrRBTree> (0));
        ASSERT_EQ (0, testGrowInPlace<cookmem::PtrRBTree> (cookmem::MAX_PADDING_SIZE));
        ASSERT_EQ (0, testGrowInPlace<cookmem::PtrAVLTree> (1));
        ASSERT_EQ (0, testRandomReallocate ());
        ASSERT_EQ (0, testPreferBetterFit<cookmem::PtrRBTree> ());
        ASSERT_EQ (0, testPreferBetterFit<cookmem::PtrAVLTree> ());
        ASSERT_EQ (0, testImmediateRelease ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }

    return 0;
}
