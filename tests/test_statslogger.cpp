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
#include <cookmem/logger.h>

int
main ()
{
    cookmem::StatsLogger logger;
    int allocation;
    int segment;
    int segment2;

    logger.logGetSegment (&segment, 128);
    logger.logCoalesce ();
    logger.logCoalesce ();
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
        stats.getNumCoalesces () != 2 ||
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
        logger.logError (&allocation, cookmem::MEM_ERROR_PADDING);
    }
    catch (const cookmem::Exception& error)
    {
        return error.getError () == cookmem::MEM_ERROR_PADDING ? 0 : 2;
    }
    return 3;
}
