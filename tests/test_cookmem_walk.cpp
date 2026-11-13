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
// Tests for forEachAllocation, which visits the allocations not freed.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

#include <cookmem.h>
#include "unittest.h"

typedef std::map<void*, std::size_t> AllocationMap;

/**
 * Collect the allocations of a context into a map, and check that they are
 * visited in address order within a segment and that none is visited twice.
 */
template<class Context>
static AllocationMap
walk (const Context& memCtx, std::size_t& count)
{
    AllocationMap found;
    std::size_t visited = 0;
    count = memCtx.forEachAllocation ([&found, &visited] (void* ptr, std::size_t size) {
        found[ptr] = size;
        ++visited;
    });
    if (visited != count || found.size () != count)
    {
        // Makes the test fail at the comparison of the caller.
        found.clear ();
        count = ~static_cast<std::size_t> (0);
    }
    return found;
}

/**
 * Allocate blocks of various sizes, including ones in their own segments,
 * free some of them, and check that exactly the others are visited, with
 * the sizes reported by getSize ().
 */
template<class TreeType>
static int
testWalk (std::uint8_t paddingSize)
{
    cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::NoActionMemLogger, TreeType> memCtx (paddingSize);

    std::size_t count;
    ASSERT_EQ (0u, walk (memCtx, count).size ());
    ASSERT_EQ (0u, count);

    // Small, large, and over the immediate release threshold (a segment of
    // its own).  Enough of them for more than one segment.
    static const std::size_t SIZES[] = { 0, 1, 7, 24, 100, 255, 300, 1000, 5000, 70000, 300000 };
    const std::size_t numSizes = sizeof (SIZES) / sizeof (SIZES[0]);

    std::vector<void*> ptrs;
    for (int round = 0; round < 20; ++round)
    {
        for (std::size_t i = 0; i < numSizes; ++i)
        {
            void* ptr = memCtx.allocate (SIZES[i]);
            ASSERT_NE (nullptr, ptr);
            ptrs.push_back (ptr);
        }
    }
    // Aligned allocations report the pointer returned to the user.
    for (std::size_t alignment = 64; alignment <= 4096; alignment *= 4)
    {
        void* ptr = memCtx.allocateAligned (100, alignment);
        ASSERT_NE (nullptr, ptr);
        ASSERT_EQ (0u, reinterpret_cast<std::uintptr_t> (ptr) % alignment);
        ptrs.push_back (ptr);
    }

    // Free every third one, and resize some of the others.
    AllocationMap expected;
    for (std::size_t i = 0; i < ptrs.size (); ++i)
    {
        if (i % 3 == 0)
        {
            memCtx.deallocate (ptrs[i]);
            continue;
        }
        if (i % 3 == 1)
        {
            ptrs[i] = memCtx.reallocate (ptrs[i], i * 10);
            ASSERT_NE (nullptr, ptrs[i]);
        }
        expected[ptrs[i]] = memCtx.getSize (ptrs[i]);
    }
    memCtx.validate ();

    AllocationMap found = walk (memCtx, count);
    ASSERT_EQ (expected.size (), count);
    ASSERT_EQ (true, expected == found);

    // With padding, the exact sizes are stored.
    if (paddingSize != 0)
    {
        for (std::size_t i = 1; i < ptrs.size (); i += 3)
        {
            ASSERT_EQ (i * 10, found[ptrs[i]]);
        }
    }

    // Free the rest.
    for (AllocationMap::iterator it = expected.begin (); it != expected.end (); ++it)
    {
        memCtx.deallocate (it->first);
    }
    ASSERT_EQ (0u, walk (memCtx, count).size ());
    ASSERT_EQ (0u, count);
    return 0;
}

static std::size_t g_totalSize;

static void
addSize (void*, std::size_t size)
{
    g_totalSize += size;
}

/**
 * A plain function works as the visitor too, and a const context can be
 * walked.
 */
static int
testFunctionVisitor ()
{
    cookmem::SimpleMemContext<> memCtx (cookmem::MAX_PADDING_SIZE);
    void* a = memCtx.allocate (10);
    void* b = memCtx.allocate (2000);
    ASSERT_NE (nullptr, a);
    ASSERT_NE (nullptr, b);

    const cookmem::SimpleMemContext<>& constCtx = memCtx;
    g_totalSize = 0;
    ASSERT_EQ (2u, constCtx.forEachAllocation (addSize));
    ASSERT_EQ (2010u, g_totalSize);

    memCtx.deallocate (a);
    memCtx.deallocate (b);
    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, testWalk<cookmem::PtrRBTree> (0));
        ASSERT_EQ (0, testWalk<cookmem::PtrRBTree> (cookmem::MAX_PADDING_SIZE));
        ASSERT_EQ (0, testWalk<cookmem::PtrAVLTree> (1));
        ASSERT_EQ (0, testFunctionVisitor ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }

    return 0;
}
