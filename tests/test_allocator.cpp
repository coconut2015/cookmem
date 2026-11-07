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
#include <cookmem/allocator.h>

#include <list>
#include <limits>
#include <new>
#include <vector>

#include "unittest.h"

struct StaticAllocatorTestTag;
typedef cookmem::SimpleMemContext<cookmem::MmapArena> ScopedContext;
typedef cookmem::Allocator<
    int,
    cookmem::TLSContextProvider<
        ScopedContext,
        StaticAllocatorTestTag> > StaticIntAllocator;

struct TrackingArena
{
    cookmem::MmapArena arena;
    static std::size_t allocatedSegments;
    static std::size_t freedSegments;

    void*
    getSegment (std::size_t& size)
    {
        void* ptr = arena.getSegment (size);
        if (ptr != nullptr)
        {
            ++allocatedSegments;
        }
        return ptr;
    }

    bool
    freeSegment (void* ptr, std::size_t size)
    {
        const bool error = arena.freeSegment (ptr, size);
        if (!error)
        {
            ++freedSegments;
        }
        return error;
    }
};

std::size_t TrackingArena::allocatedSegments = 0;
std::size_t TrackingArena::freedSegments = 0;

struct ScopedAllocatorTestTag;
typedef cookmem::SimpleMemContext<TrackingArena> TrackedContext;
typedef cookmem::Allocator<
    int,
    cookmem::TLSContextProvider<TrackedContext, ScopedAllocatorTestTag> > ScopedIntAllocator;

struct SharedArenaTestTag;
typedef cookmem::CachedMemContext<TrackingArena> SharedArenaContext;
typedef cookmem::Allocator<
    int,
    cookmem::TLSContextProvider<SharedArenaContext, SharedArenaTestTag> > SharedArenaIntAllocator;

static int
test1 ()
{
    typedef cookmem::Allocator<int> IntAllocator;
    IntAllocator::context_type context;
    IntAllocator allocator (context);

    std::vector<int, IntAllocator> values (allocator);
    values.push_back (42);
    values.push_back (7);

    std::list<int, IntAllocator> linkedValues (allocator);
    linkedValues.push_back (values[0]);
    linkedValues.push_back (values[1]);

    ASSERT_EQ (2UL, values.size ());
    ASSERT_EQ (2UL, linkedValues.size ());
    ASSERT_EQ (42, linkedValues.front ());
    ASSERT_EQ (7, linkedValues.back ());
    return 0;
}

static int
testAllocatorOperations ()
{
    typedef cookmem::Allocator<int> IntAllocator;
    typedef IntAllocator::rebind<char>::other CharAllocator;
    IntAllocator::context_type context;
    IntAllocator allocator (context);
    IntAllocator copiedAllocator (allocator);
    CharAllocator reboundAllocator (allocator);
    IntAllocator::context_type otherContext;
    IntAllocator otherAllocator (otherContext);

    ASSERT_EQ (&context, &copiedAllocator.getContext ());
    ASSERT_EQ (&context, &reboundAllocator.getContext ());
    ASSERT_EQ (true, allocator == copiedAllocator);
    ASSERT_EQ (true, allocator == reboundAllocator);
    ASSERT_EQ (false, allocator == otherAllocator);
    ASSERT_EQ (false, allocator != copiedAllocator);
    ASSERT_EQ (true, allocator != otherAllocator);
    ASSERT_EQ (false, IntAllocator::is_always_equal::value);
    ASSERT_EQ (false, IntAllocator::propagate_on_container_copy_assignment::value);
    ASSERT_EQ (false, IntAllocator::propagate_on_container_move_assignment::value);
    ASSERT_EQ (false, IntAllocator::propagate_on_container_swap::value);

    int value;
    allocator.construct (&value, 42);
    ASSERT_EQ (42, value);
    allocator.destroy (&value);

    char* bytes = reboundAllocator.allocate (4);
    ASSERT_NE (nullptr, bytes);
    reboundAllocator.deallocate (bytes, 4);

    int* copiedValue = copiedAllocator.allocate (1);
    ASSERT_NE (nullptr, copiedValue);
    copiedAllocator.deallocate (copiedValue, 1);
    return 0;
}

static int
testStaticProvider ()
{
    typedef StaticIntAllocator::rebind<char>::other CharAllocator;
    StaticIntAllocator allocator;
    CharAllocator reboundAllocator;

    ASSERT_EQ (&allocator.getContext (), &reboundAllocator.getContext ());
    ASSERT_EQ (true, allocator == reboundAllocator);
    ASSERT_EQ (false, StaticIntAllocator::is_always_equal::value);
    ASSERT_EQ (true, StaticIntAllocator::propagate_on_container_copy_assignment::value);
    ASSERT_EQ (true, StaticIntAllocator::propagate_on_container_move_assignment::value);
    ASSERT_EQ (true, StaticIntAllocator::propagate_on_container_swap::value);

    int* value = allocator.allocate (1);
    ASSERT_NE (nullptr, value);
    allocator.deallocate (value, 1);
    return 0;
}

static int
testScopedTLSContext ()
{
    typedef cookmem::TLSContextProvider<TrackedContext, ScopedAllocatorTestTag> Provider;

    TrackingArena::allocatedSegments = 0;
    TrackingArena::freedSegments = 0;
    TrackedContext& threadContext = Provider::getThreadContext ();
    ASSERT_EQ (&threadContext, &Provider::getThreadContext ());
    ASSERT_EQ (&threadContext, &Provider::getActiveContext ());

    {
        cookmem::TLSContextScope<TrackedContext, ScopedAllocatorTestTag> scope;
        std::vector<int, ScopedIntAllocator> values;
        ASSERT_EQ (&scope.getContext (), &Provider::getActiveContext ());
        ASSERT_NE (&threadContext, &Provider::getActiveContext ());
        for (int i = 0; i < 128; ++i)
        {
            values.push_back (i);
        }

        ScopedIntAllocator outerAllocator;
        ASSERT_EQ (&scope.getContext (), &outerAllocator.getContext ());
        ASSERT_EQ (128UL, values.size ());

        {
            cookmem::TLSContextScope<TrackedContext, ScopedAllocatorTestTag> nestedScope;
            ScopedIntAllocator nestedAllocator;
            ASSERT_EQ (&nestedScope.getContext (), &Provider::getActiveContext ());
            ASSERT_NE (&outerAllocator.getContext (), &nestedAllocator.getContext ());
        }

        ASSERT_EQ (&scope.getContext (), &Provider::getActiveContext ());
        ScopedIntAllocator restoredAllocator;
        ASSERT_EQ (&outerAllocator.getContext (), &restoredAllocator.getContext ());
    }

    ASSERT_EQ (&threadContext, &Provider::getActiveContext ());
    ASSERT_EQ (&threadContext, &Provider::getThreadContext ());
    ASSERT_NE (0UL, TrackingArena::allocatedSegments);
    ASSERT_EQ (TrackingArena::allocatedSegments, TrackingArena::freedSegments);
    return 0;
}

static int
testScopedSharedArena ()
{
    TrackingArena::allocatedSegments = 0;

    SharedArenaIntAllocator threadAllocator;
    int* ptr = threadAllocator.allocate (128);
    ASSERT_NE (nullptr, ptr);
    threadAllocator.getContext ().releaseAll ();
    const std::size_t allocatedSegments = TrackingArena::allocatedSegments;

    {
        cookmem::TLSContextScope<SharedArenaContext, SharedArenaTestTag> scope (true);
        SharedArenaIntAllocator scopeAllocator;
        ASSERT_EQ (&scope.getContext (), &scopeAllocator.getContext ());

        ptr = scopeAllocator.allocate (128);
        ASSERT_NE (nullptr, ptr);
        ASSERT_EQ (allocatedSegments, TrackingArena::allocatedSegments);
        scopeAllocator.deallocate (ptr, 128);
    }

    ptr = threadAllocator.allocate (128);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (allocatedSegments, TrackingArena::allocatedSegments);
    threadAllocator.deallocate (ptr, 128);
    threadAllocator.getContext ().releaseAll ();
    return 0;
}

template<class Allocator>
static bool
throwsBadAlloc (Allocator& allocator, typename Allocator::size_type count)
{
    try
    {
        allocator.allocate (count);
    }
    catch (const std::bad_alloc&)
    {
        return true;
    }
    return false;
}

static int
test2 ()
{
    typedef cookmem::Allocator<int> IntAllocator;
    IntAllocator::context_type context;
    IntAllocator allocator (context);

    ASSERT_EQ (nullptr, allocator.allocate (0));
    return 0;
}

static int
test3 ()
{
    typedef cookmem::Allocator<int> IntAllocator;
    IntAllocator::context_type context;
    IntAllocator allocator (context);

    ASSERT_EQ (true, throwsBadAlloc (allocator, allocator.max_size ()));
    ASSERT_EQ (true,
               throwsBadAlloc (allocator,
                               (std::numeric_limits<IntAllocator::size_type>::max) ()));
    return 0;
}

static int
testContainerSwap ()
{
    typedef cookmem::Allocator<int> IntAllocator;
    IntAllocator::context_type context;
    IntAllocator allocator (context);
    std::vector<int, IntAllocator> first (allocator);
    std::vector<int, IntAllocator> second (allocator);
    first.push_back (1);
    second.push_back (2);

    first.swap (second);

    ASSERT_EQ (2, first.front ());
    ASSERT_EQ (1, second.front ());
    return 0;
}

int
main ()
{
    try
    {
        ASSERT_EQ (0, test1 ());
        ASSERT_EQ (0, testAllocatorOperations ());
        ASSERT_EQ (0, testStaticProvider ());
        ASSERT_EQ (0, testScopedTLSContext ());
        ASSERT_EQ (0, testScopedSharedArena ());
        ASSERT_EQ (0, test2 ());
        ASSERT_EQ (0, test3 ());
        ASSERT_EQ (0, testContainerSwap ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    return 0;
}
