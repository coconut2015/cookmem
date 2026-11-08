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

#include <cookmem.h>

typedef cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::StatsLogger> MemCtx;

static void
printSegments (const char* when, MemCtx& memCtx)
{
    std::cout << when << ": " << memCtx.getLogger ().getStats ().getCurrentSegments ()
              << " segments, footprint " << memCtx.getFootprint () << " bytes" << std::endl;
}

int
main (int argc, const char* argv[])
{
    MemCtx memCtx;

    // Each 200 KB block needs its own memory segment.
    const int numBlocks = 8;
    void* blocks[numBlocks];
    for (int i = 0; i < numBlocks; ++i)
    {
        blocks[i] = memCtx.allocate (200 * 1024);
    }
    printSegments ("After allocating", memCtx);

    // Freeing the blocks does not release the segments right away.  The
    // memory pool keeps them for later allocations, and releases segments
    // that are entirely free every few thousand deallocations (4095 by
    // default).
    for (int i = 0; i < numBlocks; ++i)
    {
        memCtx.deallocate (blocks[i]);
    }
    printSegments ("After freeing", memCtx);

    // Release the empty segments now.
    memCtx.trimEmptySegments ();
    printSegments ("After trimEmptySegments ()", memCtx);

    // Alternatively, release empty segments more often.  With an interval of
    // 1, the check is done on every deallocation.  This keeps the footprint
    // small, at the cost of releasing and obtaining segments more often.  With
    // CachedMemContext, released segments are cached by the arena, so this
    // is cheaper.
    memCtx.setTrimEmptySegmentsInterval (1);
    for (int i = 0; i < numBlocks; ++i)
    {
        blocks[i] = memCtx.allocate (200 * 1024);
    }
    printSegments ("After allocating again", memCtx);
    for (int i = 0; i < numBlocks; ++i)
    {
        memCtx.deallocate (blocks[i]);
    }
    printSegments ("After freeing with interval 1", memCtx);

    return 0;
}
