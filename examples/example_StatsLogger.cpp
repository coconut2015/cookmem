/*
 * Copyright (c) 2026 Heng Yuan
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
#include <string>
#include <vector>

#include <cookmem.h>

int
main (int argc, const char* argv[])
{
    // Use StatsLogger instead of the default NoActionMemLogger to gather
    // statistics on allocations and memory segments.
    typedef cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::StatsLogger> MemCtx;
    MemCtx memCtx;

    std::vector<void*> ptrs;
    for (int i = 0; i < 1000; ++i)
    {
        ptrs.push_back (memCtx.allocate (16 + i));
    }
    for (std::size_t i = 0; i < ptrs.size (); i += 2)
    {
        memCtx.deallocate (ptrs[i]);
    }

    const cookmem::MemStatistics& stats = memCtx.getLogger ().getStats ();

    // Expect 1000 allocations and 500 deallocations.
    std::cout << "Allocations: " << stats.getNumAllocations () << std::endl;
    std::cout << "Deallocations: " << stats.getNumDeallocations () << std::endl;

    // Sizes are the sizes requested by the user, not the chunk sizes.
    std::cout << "Bytes currently allocated: " << stats.getCurrentAllocated () << std::endl;
    std::cout << "Peak bytes allocated: " << stats.getPeakAllocated () << std::endl;

    std::cout << "Memory segments in use: " << stats.getCurrentSegments ()
              << " (" << stats.getCurrentSegmentSize () << " bytes)" << std::endl;

    // Print everything, including how the allocations were satisfied.
    stats.printStats ();

    for (std::size_t i = 1; i < ptrs.size (); i += 2)
    {
        memCtx.deallocate (ptrs[i]);
    }

    return 0;
}
