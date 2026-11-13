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
#include <vector>

#include <cookmem.h>

typedef cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::StatsLogger> StatsContext;

/**
 * Check that the bytes StatsLogger counts as allocated match the sizes the
 * context reports, which needs the pool to log the same size for an
 * allocation, a reallocation and a deallocation.  Without the exact size,
 * the size is rounded up to the alignment.
 */
static bool
statsMatch (StatsContext& memCtx, const std::vector<void*>& ptrs)
{
    std::size_t total = 0;
    for (void* ptr : ptrs)
    {
        total += memCtx.getSize (ptr);
    }
    const cookmem::MemStatistics& stats = memCtx.getLogger ().getStats ();
    return stats.getCurrentAllocated () == total &&
           stats.getTotalAllocated () - stats.getTotalDeallocated () == total;
}

/**
 * The bytes in use, read through a const context: getLogger (), getPool ()
 * and getArena () have const versions.
 */
static std::size_t
currentAllocated (const StatsContext& memCtx)
{
    (void) memCtx.getArena ();
    (void) memCtx.getPool ().getFootprint ();
    return memCtx.getLogger ().getStats ().getCurrentAllocated ();
}

static int
testContextSizes (bool storingExactSize)
{
    StatsContext memCtx;
    memCtx.setStoringExactSize (storingExactSize);

    std::vector<void*> ptrs;
    ptrs.push_back (memCtx.allocate (100));
    ptrs.push_back (memCtx.allocate (3));
    ptrs.push_back (memCtx.allocateAligned (1000, 64));
    if (!statsMatch (memCtx, ptrs))
    {
        return 1;
    }

    // Grow and shrink in place.  The last allocation is followed by free
    // memory, so it can grow.
    void* last = memCtx.allocate (100);
    ptrs.push_back (last);
    if (!memCtx.resizeInPlace (last, 300) || !statsMatch (memCtx, ptrs))
    {
        return 2;
    }
    if (!memCtx.resizeInPlace (last, 50) || !statsMatch (memCtx, ptrs))
    {
        return 3;
    }
    ptrs.back () = memCtx.reallocate (last, 100000);
    if (!statsMatch (memCtx, ptrs))
    {
        return 4;
    }

    if (currentAllocated (memCtx) == 0)
    {
        return 5;
    }
    for (void* ptr : ptrs)
    {
        memCtx.deallocate (ptr);
    }
    ptrs.clear ();
    if (currentAllocated (memCtx) != 0)
    {
        return 6;
    }
    return statsMatch (memCtx, ptrs) ? 0 : 7;
}

int
main ()
{
    if (testContextSizes (false) != 0 || testContextSizes (true) != 0)
    {
        return 4;
    }

    cookmem::StatsLogger logger;
    int allocation;
    int segment;
    int segment2;

    logger.logGetSegment (&segment, 128);
    logger.logTrimEmptySegments ();
    logger.logTrimEmptySegments ();
    logger.logGetSegment (nullptr, 256);
    logger.logGetSegment (&segment2, 64);
    logger.logFreeSegment (nullptr, 512);
    logger.logFreeSegment (&segment, 128);
    logger.logFreeSegment (&segment2, 64);
    logger.logAllocation (&allocation, 32, cookmem::MEM_ALLOC_SMALL);
    logger.logAllocation (nullptr, 64, cookmem::MEM_ALLOC_BIG);
    logger.logDeallocation (nullptr, 64);
    logger.logReallocation (&allocation, 32, 20);
    logger.logReallocation (&allocation, 20, 40);
    logger.logDeallocation (&allocation, 40);

    const cookmem::MemStatistics& stats = logger.getStats ();
    if (stats.getNumAllocations () != 1 ||
        stats.getNumAllocations (cookmem::MEM_ALLOC_SMALL) != 1 ||
        stats.getNumAllocations (cookmem::MEM_ALLOC_BIG) != 0 ||
        stats.getNumDeallocations () != 1 ||
        stats.getNumGetSegments () != 2 ||
        stats.getNumFreeSegments () != 2 ||
        stats.getNumTrimEmptySegments () != 2 ||
        stats.getTotalAllocated () != 52 ||
        stats.getTotalDeallocated () != 52 ||
        stats.getTotalGetSegmentSize () != 192 ||
        stats.getTotalFreeSegmentSize () != 192 ||
        stats.getCurrentAllocated () != 0 ||
        stats.getPeakAllocated () != 40 ||
        stats.getCurrentSegments () != 0 ||
        stats.getPeakSegments () != 2 ||
        stats.getCurrentSegmentSize () != 0 ||
        stats.getPeakSegmentSize () != 192)
    {
        return 1;
    }

    try
    {
        logger.logError (&allocation, cookmem::MEM_ERROR_PADDING, "test");
    }
    catch (const cookmem::Exception& error)
    {
        return error.getError () == cookmem::MEM_ERROR_PADDING ? 0 : 2;
    }
    return 3;
}
