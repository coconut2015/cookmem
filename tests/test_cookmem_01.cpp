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
#include <cstring>
#include <iostream>

#include <cookmem.h>
#include "unittest.h"

static int
test1 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);

    return 0;
}

static int
test2 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* ptr[10000];

    int size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    for (int i = 0; i < 10000; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    for (int i = 0; i < 10000; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    return 0;
}

static int
test3 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* ptr[300];

    int size = 300;
    for (int i = 0; i < 300; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    for (int i = 0; i < 300; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    size = 300;
    for (int i = 0; i < 300; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    for (int i = 0; i < 300; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    return 0;
}

static int
test4 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    memCtx.setTrimEmptySegmentsInterval (2);

    void* ptr[3];

    int size = 8;
    for (int i = 0; i < 3; ++i)
    {
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    memCtx.validate ();

    for (int i = 2; i >= 0; --i)
    {
        memCtx.deallocate (ptr[i]);
    }

    memCtx.validate ();

    return 0;
}

static int
test5 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;
    const int NUM = 3;

    memCtx.setTrimEmptySegmentsInterval (2);

    void* ptr[NUM];

    int size = 8;
    for (int i = 0; i < NUM; ++i)
    {
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    memCtx.validate ();

    for (int i = 0; i < NUM; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    memCtx.validate ();

    size = 8;
    for (int i = 0; i < NUM; ++i)
    {
        ptr[i] = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr[i]);
    }

    memCtx.validate ();

    for (int i = 0; i < NUM; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    memCtx.validate ();

    return 0;
}

static int
test6 ()
{
    cookmem::MallocArena arena (256);
    cookmem::NoActionMemLogger logger;
    cookmem::MemContext<cookmem::MallocArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

    memCtx.getPool ().setFootprintLimit (256);
    memCtx.setTrimEmptySegmentsInterval (100);

    void* first = memCtx.allocate (8);
    void* second = memCtx.allocate (8);
    void* third = memCtx.allocate (8);
    void* last = memCtx.allocate (112);
    ASSERT_NE (nullptr, first);
    ASSERT_NE (nullptr, second);
    ASSERT_NE (nullptr, third);
    ASSERT_NE (nullptr, last);

    memCtx.deallocate (first);
    memCtx.deallocate (second);
    memCtx.validate ();

    void* merged = memCtx.allocate (40);
    ASSERT_EQ (first, merged);

    memCtx.deallocate (third);
    memCtx.deallocate (last);
    memCtx.deallocate (merged);
    memCtx.validate ();

    first = memCtx.allocate (8);
    second = memCtx.allocate (8);
    third = memCtx.allocate (8);
    last = memCtx.allocate (112);
    ASSERT_NE (nullptr, first);
    ASSERT_NE (nullptr, second);
    ASSERT_NE (nullptr, third);
    ASSERT_NE (nullptr, last);

    memCtx.deallocate (third);
    memCtx.deallocate (second);
    memCtx.validate ();

    merged = memCtx.allocate (40);
    ASSERT_EQ (second, merged);

    memCtx.deallocate (first);
    memCtx.deallocate (last);
    memCtx.deallocate (merged);
    memCtx.validate ();

    return 0;
}

/**
 * Free memory in allocation order, so that each freed chunk merges into the
 * free chunk before it.  This exercises growing free chunks in place, both
 * for the designated victim and for chunks in the tree bins.
 */
template<class MemCtx>
static int
freeInAllocationOrder (MemCtx& memCtx)
{
    const int count = 2000;
    void* ptrs[count];

    for (int round = 0; round < 3; ++round)
    {
        for (int i = 0; i < count; ++i)
        {
            // Mostly small blocks, with a large block every 50 to break the
            // merged chunks into different tree bins.
            ptrs[i] = memCtx.allocate ((i % 50 == 49) ? 3000 : 24 + (i % 3) * 40);
            ASSERT_NE (nullptr, ptrs[i]);
        }
        memCtx.validate ();

        // Keep every 400th block, so the merged chunks stay in the bins
        // instead of becoming one empty segment.
        for (int i = 0; i < count; ++i)
        {
            if (i % 400 != 399)
            {
                memCtx.deallocate (ptrs[i]);
                ptrs[i] = nullptr;
            }
            if (i % 97 == 0)
            {
                memCtx.validate ();
            }
        }
        memCtx.validate ();

        for (int i = 0; i < count; ++i)
        {
            memCtx.deallocate (ptrs[i]);
        }
        memCtx.validate ();
    }

    return 0;
}

static int
test7 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> rbCtx;
    ASSERT_EQ (0, freeInAllocationOrder (rbCtx));

    cookmem::SimpleMemContext<cookmem::MallocArena, cookmem::NoActionMemLogger, cookmem::PtrAVLTree> avlCtx;
    ASSERT_EQ (0, freeInAllocationOrder (avlCtx));

    return 0;
}

/**
 * Random allocation sizes and free orders, checked with validate ().  The
 * contents of each block are also checked before it is freed.
 */
template<class MemCtx>
static int
randomAllocations (MemCtx& memCtx, unsigned int seed)
{
    const int maxBlocks = 600;
    unsigned char* ptrs[maxBlocks];
    std::size_t sizes[maxBlocks];
    int numBlocks = 0;

    for (int round = 0; round < 8; ++round)
    {
        // allocate
        const int numAllocs = 100 + (int) ((seed >> 8) % 400);
        for (int i = 0; i < numAllocs && numBlocks < maxBlocks; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            const unsigned int r = seed >> 8;
            const std::size_t size = (r % 8 == 0) ? 1 + r % 70000 : 1 + r % 2000;
            unsigned char* ptr = static_cast<unsigned char*> (memCtx.allocate (size));
            ASSERT_NE (nullptr, ptr);
            std::memset (ptr, (unsigned char) size, size);
            ptrs[numBlocks] = ptr;
            sizes[numBlocks] = size;
            ++numBlocks;
        }
        memCtx.validate ();

        // free some of the blocks, in allocation order, in reverse order or
        // in random order
        const int numFrees = numBlocks * (1 + round % 3) / 4;
        for (int i = 0; i < numFrees; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            int index;
            switch (round % 3)
            {
                case 0: index = 0; break;
                case 1: index = numBlocks - 1; break;
                default: index = (int) ((seed >> 8) % numBlocks); break;
            }

            for (std::size_t j = 0; j < sizes[index]; ++j)
            {
                ASSERT_EQ ((unsigned char) sizes[index], ptrs[index][j]);
            }
            memCtx.deallocate (ptrs[index]);

            // keep the remaining blocks in allocation order
            --numBlocks;
            for (int j = index; j < numBlocks; ++j)
            {
                ptrs[j] = ptrs[j + 1];
                sizes[j] = sizes[j + 1];
            }

            if (i % 23 == 0)
            {
                memCtx.validate ();
            }
        }
        memCtx.validate ();
    }

    for (int i = 0; i < numBlocks; ++i)
    {
        memCtx.deallocate (ptrs[i]);
    }
    memCtx.validate ();

    return 0;
}

static int
test8 ()
{
    for (unsigned int seed = 1; seed <= 6; ++seed)
    {
        cookmem::SimpleMemContext<cookmem::MallocArena> rbCtx;
        ASSERT_EQ (0, randomAllocations (rbCtx, seed * 7919u));

        cookmem::SimpleMemContext<cookmem::MallocArena, cookmem::NoActionMemLogger, cookmem::PtrAVLTree> avlCtx;
        ASSERT_EQ (0, randomAllocations (avlCtx, seed * 7919u));
    }

    return 0;
}

/**
 * The footprint goes down when empty segments are released, so a context
 * with a footprint limit keeps working when memory is freed and reused.
 */
static int
test9 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;
    const std::size_t limit = 1024 * 1024;

    memCtx.setFootprintLimit (limit);
    ASSERT_EQ (limit, memCtx.getFootprintLimit ());
    ASSERT_EQ (std::size_t (0), memCtx.getFootprint ());

    // Each block needs its own segment.
    void* ptrs[4];
    for (int i = 0; i < 4; ++i)
    {
        ptrs[i] = memCtx.allocate (200 * 1024);
        ASSERT_NE (nullptr, ptrs[i]);
    }
    const std::size_t footprint = memCtx.getFootprint ();
    ASSERT_EQ (true, footprint >= 4 * 200 * 1024);
    ASSERT_EQ (true, footprint <= limit);
    ASSERT_EQ (footprint, memCtx.getMaxFootprint ());

    for (int i = 0; i < 4; ++i)
    {
        memCtx.deallocate (ptrs[i]);
    }
    memCtx.trimEmptySegments ();
    ASSERT_EQ (std::size_t (0), memCtx.getFootprint ());
    ASSERT_EQ (footprint, memCtx.getMaxFootprint ());
    memCtx.validate ();

    // Allocate and free repeatedly, releasing the segment every time.  The
    // footprint must not keep growing toward the limit.
    memCtx.setTrimEmptySegmentsInterval (1);
    for (int round = 0; round < 50; ++round)
    {
        void* ptr = memCtx.allocate (500 * 1024);
        ASSERT_NE (nullptr, ptr);
        memCtx.deallocate (ptr);
        ASSERT_EQ (std::size_t (0), memCtx.getFootprint ());
    }
    memCtx.validate ();

    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, test1 ());
        ASSERT_EQ (0, test2 ());
        ASSERT_EQ (0, test3 ());
        ASSERT_EQ (0, test4 ());
        ASSERT_EQ (0, test5 ());
        ASSERT_EQ (0, test6 ());
        ASSERT_EQ (0, test7 ());
        ASSERT_EQ (0, test8 ());
        ASSERT_EQ (0, test9 ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    return 0;
}
