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

static int
test1 ()
{
    typedef cookmem::CookMemAllocator<int> IntAllocator;
    IntAllocator allocator;

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
    typedef cookmem::CookMemAllocator<int> IntAllocator;
    typedef IntAllocator::rebind<char>::other CharAllocator;
    IntAllocator allocator;
    IntAllocator copiedAllocator (allocator);
    CharAllocator reboundAllocator (allocator);

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
    typedef cookmem::CookMemAllocator<int> IntAllocator;
    IntAllocator allocator;

    ASSERT_EQ (nullptr, allocator.allocate (0));
    return 0;
}

static int
test3 ()
{
    typedef cookmem::CookMemAllocator<int> IntAllocator;
    IntAllocator allocator;

    ASSERT_EQ (true, throwsBadAlloc (allocator, allocator.max_size ()));
    ASSERT_EQ (true,
               throwsBadAlloc (allocator,
                               (std::numeric_limits<IntAllocator::size_type>::max) ()));
    return 0;
}

int
main ()
{
    try
    {
        ASSERT_EQ (0, test1 ());
        ASSERT_EQ (0, testAllocatorOperations ());
        ASSERT_EQ (0, test2 ());
        ASSERT_EQ (0, test3 ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    return 0;
}
