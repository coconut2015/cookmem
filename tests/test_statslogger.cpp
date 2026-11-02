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

    logger.logGetSegment (&segment, 128);
    logger.logFreeSegment (&segment, 64);
    logger.logAllocation (&allocation, 32, cookmem::MEM_ALLOC_SMALL);
    logger.logAllocation (nullptr, 64, cookmem::MEM_ALLOC_BIG);
    logger.logReallocation (&allocation, 32, 20);
    logger.logReallocation (&allocation, 20, 28);
    logger.logDeallocation (&allocation, 28);

    if (logger.getNumAllocations () != 1 ||
        logger.getNumAllocations (cookmem::MEM_ALLOC_SMALL) != 1 ||
        logger.getNumAllocations (cookmem::MEM_ALLOC_BIG) != 0 ||
        logger.getNumDeallocations () != 1 ||
        logger.getNumGetSegments () != 1 ||
        logger.getNumFreeSegments () != 1 ||
        logger.getTotalAllocated () != 40 ||
        logger.getTotalDeallocated () != 40 ||
        logger.getTotalGetSegmentSize () != 128 ||
        logger.getTotalFreeSegmentSize () != 64)
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
