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
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));
	cookmem::NoActionMemLogger logger;
	cookmem::MemContext< cookmem::FixedArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_EQ (true, memCtx.contains (ptr));
    ASSERT_EQ (true, memCtx.contains (ptr, true));
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (nullptr, ptr);

    ASSERT_EQ (false, memCtx.contains ((void*)&arena));

    return 0;
}

static int
test2 ()
{
    struct
    {
        char dummy;
        char buffer[64000];
    }   s;
    cookmem::FixedArena arena (s.buffer, sizeof(s.buffer));
    cookmem::NoActionMemLogger logger;
    cookmem::MemContext< cookmem::FixedArena, cookmem::NoActionMemLogger> memCtx(arena, logger);

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_EQ (true, memCtx.contains (ptr));
    ASSERT_EQ (true, memCtx.contains (ptr, true));
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (nullptr, ptr);

    ASSERT_EQ (false, memCtx.contains ((void*)&arena));

    return 0;
}

static int
test3 ()
{
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));

    std::size_t size = 3000;
    void* ptr = arena.getSegment (size);
    ASSERT_EQ (ptr, buffer);
    ASSERT_EQ (size, sizeof(buffer));
    ASSERT_EQ (nullptr, arena.getSegment (size));
    ASSERT_EQ (false, arena.freeSegment (ptr, size));
    ASSERT_EQ (true, arena.freeSegment (ptr, size));

    return 0;
}

/**
 * FixedArena::releaseAll () does nothing, and releaseAll () and
 * releaseAllToSystem () of the context return the buffer to the arena.
 */
static int
test4 ()
{
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));

    // The segment in use is not affected.
    std::size_t size = 3000;
    void* ptr = arena.getSegment (size);
    ASSERT_NE (nullptr, ptr);
    arena.releaseAll ();
    ASSERT_EQ (nullptr, arena.getSegment (size));
    ASSERT_EQ (false, arena.freeSegment (ptr, size));

    cookmem::NoActionMemLogger logger;
    cookmem::MemContext<cookmem::FixedArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    memCtx.releaseAll ();
    ASSERT_EQ (false, memCtx.contains (ptr));
    ASSERT_EQ (0u, memCtx.getFootprint ());

    // The whole buffer is available again.
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_NE (nullptr, memCtx.allocate (30000));
    memCtx.releaseAllToSystem ();
    ASSERT_EQ (false, memCtx.contains (ptr));
    ASSERT_EQ (0u, memCtx.getFootprint ());

    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (true, memCtx.contains (ptr));

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

    return 0;
}
