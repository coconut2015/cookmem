/*
 * Copyright (c) 2018-2021 Heng Yuan
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

using namespace unittest;

static void
test1 ()
{
    printf ("== %s ==\n", __FUNCTION__);

    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* p1 = memCtx.allocate (32);
    printf ("allocated %p\n", p1);

    memCtx.deallocate (p1);
    printf ("deallocated %p\n", p1);

    memCtx.getPool ().print ();

    memCtx.coalesce ();
    printf ("coalesced\n");

    memCtx.getPool ().print ();

    memCtx.validate ();
    printf ("validated\n");
}

static void
test2 ()
{
    printf ("== %s ==\n", __FUNCTION__);

    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* p1 = memCtx.allocate (32);
    void* p2 = memCtx.allocate (32);

    memCtx.getPool ().print ();

    memCtx.deallocate (p1);
    memCtx.deallocate (p2);

    memCtx.getPool ().print ();

    memCtx.coalesce ();

    memCtx.getPool ().print ();

    memCtx.validate ();
}

static void
test3 ()
{
    printf ("== %s ==\n", __FUNCTION__);

    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    memCtx.allocate (32);

    void* p1 = memCtx.allocate (32);
    void* p2 = memCtx.allocate (32);

    memCtx.getPool ().print ();

    memCtx.deallocate (p2);

    memCtx.getPool ().print ();

    memCtx.coalesce ();

    memCtx.getPool ().print ();

    memCtx.validate ();

    memCtx.deallocate (p1);
}

static void
test4 ()
{
    printf ("== %s ==\n", __FUNCTION__);

    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* p1 = memCtx.allocate (32);
    void* p2 = memCtx.allocate (32);
    void* p3 = memCtx.allocate (32);
    void* p4 = memCtx.allocate (32);
    void* p5 = memCtx.allocate (32);
    void* p6 = memCtx.allocate (32);
    void* p7 = memCtx.allocate (32);

    memCtx.getPool ().print ();

    memCtx.deallocate (p1);
    memCtx.deallocate (p2);
    memCtx.deallocate (p3);
    memCtx.deallocate (p5);
    memCtx.deallocate (p6);

    memCtx.getPool ().print ();

    memCtx.coalesce ();

    memCtx.getPool ().print ();

    memCtx.validate ();

    memCtx.deallocate (p4);
    memCtx.deallocate (p7);
}

static void
test5 ()
{
    printf ("== %s ==\n", __FUNCTION__);

    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* ptr[10000];

    int size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        assertNotNull (ptr[i], __FILE__, __LINE__);
    }
    memCtx.validate ();

    printf ("******** step 1\n");

    for (int i = 0; i < 10000; ++i)
    {
        printf ("++ id: %d\n", i);
        memCtx.deallocate (ptr[i]);
        memCtx.coalesce ();
        memCtx.validate ();
    }

    printf ("******** step 2\n");

    memCtx.validate ();

    printf ("******** step 3\n");

    memCtx.coalesce ();

    printf ("******** step 4\n");

    memCtx.validate ();
}

static void
test6 ()
{
    printf ("== %s ==\n", __FUNCTION__);

    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    void* ptr[10000];

    memCtx.getArena ().printStats ();

    int size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        assertNotNull (ptr[i], __FILE__, __LINE__);
    }
    memCtx.validate ();

    printf ("******** step 1\n");
    memCtx.getArena ().printStats ();

    for (int i = 0; i < 10000; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    printf ("******** step 2\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();

    printf ("******** step 3\n");
    memCtx.getArena ().printStats ();

    memCtx.coalesce ();

    printf ("******** step 4\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();

    printf ("******** step 5\n");
    memCtx.getArena ().printStats ();

    size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        assertNotNull (ptr[i], __FILE__, __LINE__);
        printf ("id: %d, size: %zu\n", i, memCtx.getSize (ptr[i]));
    }

    printf ("******** step 6\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();
    for (int i = 0; i < 10000; ++i)
    {
        assertEquals (true, memCtx.contains (ptr[i]), __FILE__, __LINE__);
    }

    printf ("******** step 7\n");
    memCtx.getArena ().printStats ();

    for (int i = 0; i < 10000; ++i)
    {
        memCtx.deallocate (ptr[i]);
    }

    printf ("******** step 8\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();

    printf ("******** step 9\n");
    memCtx.getArena ().printStats ();

    size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = memCtx.allocate (size);
        assertNotNull (ptr[i], __FILE__, __LINE__);
    }

    printf ("******** step 10\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();

    printf ("******** step 11\n");
    memCtx.getArena ().printStats ();

    for (int i = 0; i < 10000; ++i)
    {
        printf ("++ id: %d\n", i);
        assertEquals (true, memCtx.contains (ptr[i]), __FILE__, __LINE__);
    }

    for (int i = 0; i < 10000; ++i)
    {
        printf ("-- id: %d\n", i);
        memCtx.deallocate (ptr[i]);
        memCtx.validate ();
    }

    printf ("******** step 12\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();

    printf ("******** step 13\n");
    memCtx.getArena ().printStats ();

    memCtx.coalesce ();

    printf ("******** step 14\n");
    memCtx.getArena ().printStats ();

    memCtx.validate ();
}

unittest::TestFunction s_tests[] =
{
    test1,
    test2,
    test3,
    test4,
    test5,
    test6,
};

int
main (int argc, const char* argv[])
{
    try
    {
        std::size_t numTests = sizeof (s_tests) / sizeof (unittest::TestFunction);
        for (std::size_t i = 0; i < numTests; ++i)
        {
            s_tests[i] ();
        }
    }
    catch (UnitTestException& ex)
    {
        printf ("Exception thrown at %s:%d.\n", ex.getFile (), ex.getLine ());
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        printf ("Exception thrown at %s:%d.\n", ex.getFile (), ex.getLine ());
        return 1;
    }
    return 0;
}
