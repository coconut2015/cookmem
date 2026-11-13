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
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <cookmem.h>
#include "unittest.h"

static const std::size_t ALIGNMENTS[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 1024, 4096 };
static const std::size_t SIZES[] = { 0, 1, 7, 24, 100, 300, 1000, 5000, 70000 };

static bool
isAligned (const void* ptr, std::size_t alignment)
{
    return reinterpret_cast<std::uintptr_t> (ptr) % alignment == 0;
}

struct Block
{
    unsigned char*  ptr;
    std::size_t     size;
    unsigned char   value;
};

static void
fill (Block& block)
{
    std::memset (block.ptr, block.value, block.size);
}

static bool
check (const Block& block)
{
    for (std::size_t i = 0; i < block.size; ++i)
    {
        if (block.ptr[i] != block.value)
        {
            return false;
        }
    }
    return true;
}

/**
 * Aligned allocations of every combination of size and alignment, mixed with
 * normal allocations, are aligned, do not overlap and can be freed.
 */
template<class Context>
static int
checkAlignedAllocations (Context& memCtx)
{
    std::vector<Block> blocks;
    unsigned char value = 0;

    for (std::size_t alignment : ALIGNMENTS)
    {
        for (std::size_t size : SIZES)
        {
            Block block;
            block.ptr = static_cast<unsigned char*> (memCtx.allocateAligned (size, alignment));
            ASSERT_NE (nullptr, block.ptr);
            ASSERT_EQ (true, isAligned (block.ptr, alignment));
            ASSERT_EQ (true, isAligned (block.ptr, cookmem::ALLOCATION_ALIGNMENT));
            ASSERT_EQ (true, memCtx.contains (block.ptr, true));
            ASSERT_EQ (true, memCtx.getSize (block.ptr) >= size);
            block.size = size;
            block.value = ++value;
            fill (block);
            blocks.push_back (block);

            // A normal allocation between aligned ones.
            block.ptr = static_cast<unsigned char*> (memCtx.allocate (size + 3));
            ASSERT_NE (nullptr, block.ptr);
            block.size = size + 3;
            block.value = ++value;
            fill (block);
            blocks.push_back (block);

            memCtx.validate ();
        }
    }

    // Free every other block, then the rest.
    for (int pass = 0; pass < 2; ++pass)
    {
        for (std::size_t i = pass; i < blocks.size (); i += 2)
        {
            ASSERT_EQ (true, check (blocks[i]));
            memCtx.deallocate (blocks[i].ptr);
            memCtx.validate ();
        }
    }
    return 0;
}

static int
test1 ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> mmapContext;
    ASSERT_EQ (0, checkAlignedAllocations (mmapContext));
    // Again, with the free chunks left by the first round.
    ASSERT_EQ (0, checkAlignedAllocations (mmapContext));

    cookmem::CachedMemContext<> cachedContext;
    ASSERT_EQ (0, checkAlignedAllocations (cachedContext));

    cookmem::SimpleMemContext<cookmem::MallocArena> mallocContext;
    ASSERT_EQ (0, checkAlignedAllocations (mallocContext));

    // A misaligned buffer.
    static char buffer[4 * 1024 * 1024 + 1];
    cookmem::FixedArena fixedArena (buffer + 1, sizeof (buffer) - 1);
    cookmem::NoActionMemLogger logger;
    cookmem::MemContext<cookmem::FixedArena, cookmem::NoActionMemLogger> fixedContext (fixedArena, logger);
    ASSERT_EQ (0, checkAlignedAllocations (fixedContext));

    return 0;
}

/**
 * Invalid alignments and requests that are too large fail.
 */
static int
test2 ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx;

    // Not a power of 2.
    ASSERT_EQ (nullptr, memCtx.allocateAligned (100, 0));
    ASSERT_EQ (nullptr, memCtx.allocateAligned (100, 3));
    ASSERT_EQ (nullptr, memCtx.allocateAligned (100, 24));
    ASSERT_EQ (nullptr, memCtx.allocateAligned (100, 48));

    // Too large: the size plus the alignment does not fit in the address
    // space.
    const std::size_t maxSize = ~static_cast<std::size_t> (0);
    const std::size_t topBit = maxSize ^ (maxSize >> 1);
    ASSERT_EQ (nullptr, memCtx.allocateAligned (topBit, topBit));
    ASSERT_EQ (nullptr, memCtx.allocateAligned (maxSize - 100, 64));
    ASSERT_EQ (nullptr, memCtx.allocateAligned (maxSize, 64));

    // A footprint limit that cannot fit the request.
    memCtx.setFootprintLimit (100000);
    ASSERT_EQ (nullptr, memCtx.allocateAligned (100, 1024 * 1024));
    memCtx.setFootprintLimit (0);

    // The context still works.
    void* ptr = memCtx.allocateAligned (100, 64);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (true, isAligned (ptr, 64));
    memCtx.deallocate (ptr);
    memCtx.validate ();

    return 0;
}

/**
 * Exact sizes and padding bytes work with aligned allocations.
 */
static int
test3 ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx (cookmem::MAX_PADDING_SIZE);
    memCtx.setStoringExactSize (true);

    std::vector<void*> ptrs;
    for (std::size_t alignment : ALIGNMENTS)
    {
        for (std::size_t size : SIZES)
        {
            void* ptr = memCtx.allocateAligned (size, alignment);
            ASSERT_NE (nullptr, ptr);
            ASSERT_EQ (true, isAligned (ptr, alignment));
            ASSERT_EQ (size, memCtx.getSize (ptr));
            std::memset (ptr, 0x5a, size);
            ptrs.push_back (ptr);
        }
    }
    memCtx.validate ();

    // deallocate () throws if the padding bytes were changed.
    for (void* ptr : ptrs)
    {
        memCtx.deallocate (ptr);
    }
    memCtx.validate ();

    // Writing past the end is detected.
    unsigned char* ptr = static_cast<unsigned char*> (memCtx.allocateAligned (40, 256));
    ASSERT_NE (nullptr, ptr);
    ptr[40] = 0;
    try
    {
        memCtx.deallocate (ptr);
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_PADDING, ex.getError ());
        return 0;
    }
    return 1;
}

/**
 * Shrinking an aligned allocation keeps it in place.  Growing it may move it,
 * and the new memory has only the normal alignment.
 */
static int
test4 ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx;

    void* ptr = memCtx.allocateAligned (5000, 1024);
    ASSERT_NE (nullptr, ptr);
    std::memset (ptr, 0x33, 5000);

    void* shrunk = memCtx.reallocate (ptr, 1000);
    ASSERT_EQ (ptr, shrunk);
    memCtx.validate ();

    unsigned char* grown = static_cast<unsigned char*> (memCtx.reallocate (shrunk, 100000));
    ASSERT_NE (nullptr, grown);
    for (std::size_t i = 0; i < 1000; ++i)
    {
        ASSERT_EQ (0x33, grown[i]);
    }
    memCtx.deallocate (grown);
    memCtx.validate ();

    return 0;
}

/**
 * Random aligned and normal allocations and frees.
 */
static int
test5 ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx;

    std::vector<Block> blocks;
    std::uint32_t seed = 12345;
    unsigned char value = 0;

    for (int i = 0; i < 20000; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        const std::uint32_t r = seed >> 8;

        if (blocks.empty () || (r % 3) != 0)
        {
            Block block;
            block.size = (r >> 4) % ((r & 1) ? 300 : 20000);
            std::size_t alignment = std::size_t (1) << ((r >> 2) % 13);
            block.ptr = static_cast<unsigned char*> (memCtx.allocateAligned (block.size, alignment));
            ASSERT_NE (nullptr, block.ptr);
            ASSERT_EQ (true, isAligned (block.ptr, alignment));
            block.value = ++value;
            fill (block);
            blocks.push_back (block);
        }
        else
        {
            std::size_t index = (r >> 4) % blocks.size ();
            ASSERT_EQ (true, check (blocks[index]));
            memCtx.deallocate (blocks[index].ptr);
            blocks[index] = blocks.back ();
            blocks.pop_back ();
        }

        if ((i % 1000) == 0)
        {
            memCtx.validate ();
        }
    }

    for (Block& block : blocks)
    {
        ASSERT_EQ (true, check (block));
        memCtx.deallocate (block.ptr);
    }
    memCtx.validate ();

    return 0;
}

/**
 * A type that requires a larger alignment than ALLOCATION_ALIGNMENT.
 */
struct alignas (64) OverAligned
{
    char data[64];
};

/**
 * Allocator uses aligned allocations for over-aligned types.
 */
static int
test6 ()
{
    typedef cookmem::Allocator<OverAligned> OverAlignedAllocator;
    OverAlignedAllocator::context_type context;
    OverAlignedAllocator allocator (context);

    std::vector<OverAligned, OverAlignedAllocator> values (allocator);
    for (int i = 0; i < 1000; ++i)
    {
        values.push_back (OverAligned ());
        ASSERT_EQ (true, isAligned (values.data (), alignof (OverAligned)));
    }

    // Rebinding between normal and over-aligned types.
    cookmem::Allocator<int> intAllocator (allocator);
    OverAlignedAllocator rebound (intAllocator);
    OverAligned* ptr = rebound.allocate (3);
    ASSERT_EQ (true, isAligned (ptr, alignof (OverAligned)));
    rebound.deallocate (ptr, 3);

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
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }

    return 0;
}
