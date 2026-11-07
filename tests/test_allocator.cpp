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
typedef cookmem::Allocator<
    int,
    cookmem::TLSContextProvider<
        cookmem::CachedMemContext<>,
        StaticAllocatorTestTag> > StaticIntAllocator;

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
