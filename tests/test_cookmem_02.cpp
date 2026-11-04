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
#include <iostream>

#include <cookmem.h>
#include "unittest.h"

static int
test1 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (0, memCtx.getPaddingSize ());
    ASSERT_EQ (false, memCtx.isStoringExactSize ());

    ASSERT_EQ (0UL, memCtx.getSize (nullptr));

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (32UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);
    ASSERT_EQ (0UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (304UL, memCtx.getSize (ptr));
    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (3008UL, memCtx.getSize (ptr));
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    ASSERT_NE (nullptr, ptr);

    return 0;
}

static int
test2 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (false, memCtx.isStoringExactSize ());
    memCtx.setStoringExactSize (true);
    ASSERT_EQ (true, memCtx.isStoringExactSize ());

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 30);
    ASSERT_EQ (30UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 300);
    ASSERT_EQ (300UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (3000UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    memset (ptr, 0xff, 30000);
    ASSERT_NE (nullptr, ptr);

    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    memset (ptr, 0xff, 30000);
    ASSERT_NE (nullptr, ptr);

    return 0;
}

static int
test3 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx (4);

    ASSERT_EQ (true, memCtx.isPadding ());
    ASSERT_EQ (4, memCtx.getPaddingSize ());
    ASSERT_EQ (true, memCtx.isStoringExactSize ());

    cookmem::MemContext<cookmem::MallocArena, cookmem::NoActionMemLogger> copiedContext (memCtx);
    ASSERT_EQ (4, copiedContext.getPaddingSize ());

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 30);
    ASSERT_EQ (30UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 300);
    ASSERT_EQ (300UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (3000UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    for (size_t size = 30000; size < 30020; ++size)
    {
        ptr = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr);
        memset (ptr, 0xff, size);
        ASSERT_EQ (size, memCtx.getSize (ptr));
        memCtx.deallocate (ptr);
    }

    return 0;
}

static int
test4 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;
    void* ptr = memCtx.allocate (8);
    ASSERT_NE (nullptr, ptr);

    void* resized = memCtx.reallocate (ptr, 9);
    ASSERT_EQ (ptr, resized);
    ASSERT_EQ (true, memCtx.contains (resized, true));

    memCtx.deallocate (resized);
    memCtx.deallocate (nullptr);
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
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        std::cout << "Exception thrown at " << ex.getFile () << ":"
                  << ex.getLine () << ": " << ex.getMessage () << std::endl;
        return 1;
    }
    return 0;
}
