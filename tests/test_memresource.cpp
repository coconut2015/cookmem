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
// Tests for cookmem::MemResource, the std::pmr::memory_resource adapter.
// This test is compiled as C++17.
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory_resource>
#include <new>
#include <string>
#include <vector>

#include <cookmem.h>
#include "unittest.h"

static_assert (COOKMEM_HAS_PMR == 1, "std::pmr is expected in C++17");

typedef cookmem::SimpleMemContext<> Context;
typedef cookmem::MemResource<Context> Resource;

template<class Ctx>
static std::size_t
countAllocations (const Ctx& memCtx)
{
    return memCtx.forEachAllocation ([] (void*, std::size_t) {
    });
}

/**
 * The std::pmr containers allocate from the context, and free everything
 * when they are destroyed.
 */
static int
testContainers ()
{
    Context memCtx;
    Resource resource (memCtx);
    ASSERT_EQ (&memCtx, &resource.getContext ());

    {
        std::pmr::vector<int> vec (&resource);
        for (int i = 0; i < 10000; ++i)
        {
            vec.push_back (i);
        }
        std::pmr::map<int, std::pmr::string> map (&resource);
        for (int i = 0; i < 1000; ++i)
        {
            map[i] = std::pmr::string (static_cast<std::size_t> (i), 'x');
        }
        // The strings use the allocator of the map, and so the context.
        ASSERT_EQ (&resource, map[999].get_allocator ().resource ());

        ASSERT_NE (0u, countAllocations (memCtx));
        for (int i = 0; i < 1000; ++i)
        {
            ASSERT_EQ (static_cast<std::size_t> (i), map[i].size ());
            // The map nodes are in the context.
            ASSERT_EQ (true, memCtx.contains (&map[i]));
        }
        // A string too long for the small string buffer has an allocation
        // of its own.
        ASSERT_EQ (true, memCtx.contains (map[999].data (), true));
        ASSERT_EQ (true, memCtx.contains (vec.data (), true));
        memCtx.validate ();
    }
    ASSERT_EQ (0u, countAllocations (memCtx));

    // As the upstream of a standard resource.
    {
        std::pmr::monotonic_buffer_resource monotonic (&resource);
        std::pmr::vector<std::pmr::string> strings (&monotonic);
        for (int i = 0; i < 100; ++i)
        {
            strings.emplace_back (200, 'y');
        }
        ASSERT_NE (0u, countAllocations (memCtx));
    }
    ASSERT_EQ (0u, countAllocations (memCtx));
    return 0;
}

/**
 * Over-aligned requests get the alignment asked for.
 */
static int
testAlignment ()
{
    Context memCtx (cookmem::MAX_PADDING_SIZE);
    Resource resource (memCtx);

    std::vector<void*> ptrs;
    for (std::size_t alignment = 1; alignment <= 4096; alignment *= 2)
    {
        void* ptr = resource.allocate (100, alignment);
        ASSERT_NE (nullptr, ptr);
        ASSERT_EQ (0u, reinterpret_cast<std::uintptr_t> (ptr) % alignment);
        ASSERT_EQ (100u, memCtx.getSize (ptr));
        ptrs.push_back (ptr);
    }
    memCtx.validate ();
    std::size_t alignment = 1;
    for (std::size_t i = 0; i < ptrs.size (); ++i, alignment *= 2)
    {
        resource.deallocate (ptrs[i], 100, alignment);
    }
    ASSERT_EQ (0u, countAllocations (memCtx));
    return 0;
}

/**
 * A request the context cannot satisfy throws std::bad_alloc, and the
 * context remains usable.
 */
static int
testBadAlloc ()
{
    Context memCtx;
    memCtx.setFootprintLimit (1024 * 1024);
    Resource resource (memCtx);

    // With the default alignment, and with a larger one, which uses
    // allocateAligned ().
    static const std::size_t ALIGNMENTS[] = { alignof (std::max_align_t), 4096 };
    for (std::size_t alignment : ALIGNMENTS)
    {
        bool thrown = false;
        void* ptr = nullptr;
        try
        {
            ptr = resource.allocate (2 * 1024 * 1024, alignment);
        }
        catch (const std::bad_alloc&)
        {
            thrown = true;
        }
        ASSERT_EQ (true, thrown);
        ASSERT_EQ (nullptr, ptr);
    }

    void* ptr = resource.allocate (1000);
    ASSERT_NE (nullptr, ptr);
    resource.deallocate (ptr, 1000);
    return 0;
}

/**
 * Resources are equal when they use the same context.  Without RTTI, a
 * resource is only equal to itself.
 */
static int
testIsEqual ()
{
#if defined(__cpp_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
    const bool hasRtti = true;
#else
    const bool hasRtti = false;
#endif
    Context memCtx1;
    Context memCtx2;
    Resource resource1 (memCtx1);
    Resource resource1b (memCtx1);
    Resource resource2 (memCtx2);

    ASSERT_EQ (true, resource1 == resource1);
    // operator == checks the addresses first, so call is_equal () to check
    // a resource against itself.
    ASSERT_EQ (true, resource1.is_equal (resource1));
    ASSERT_EQ (hasRtti, resource1 == resource1b);
    ASSERT_EQ (false, resource1 == resource2);
    ASSERT_EQ (false, resource1 == *std::pmr::new_delete_resource ());

    // Memory allocated by one resource can be freed by an equal one.
    void* ptr = resource1.allocate (64);
    resource1b.deallocate (ptr, 64);
    ASSERT_EQ (0u, countAllocations (memCtx1));
    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, testContainers ());
        ASSERT_EQ (0, testAlignment ());
        ASSERT_EQ (0, testBadAlloc ());
        ASSERT_EQ (0, testIsEqual ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }

    return 0;
}
