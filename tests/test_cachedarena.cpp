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

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

#define NUM_ENTRIES 9

static int
test1 ()
{
    cookmem::MallocArena arena;
    cookmem::CachedArena<cookmem::MallocArena> cachedArena (arena);
    cookmem::NoActionMemLogger logger;

    {
        cookmem::MemContext<cookmem::CachedArena<cookmem::MallocArena>, cookmem::NoActionMemLogger> memCtx (cachedArena, logger);

        void* ptrs[NUM_ENTRIES];

        std::size_t size = 3;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            size *= 10;
            ptrs[i] = memCtx.allocate (size);
            ASSERT_EQ (true, memCtx.contains (ptrs[i]));
            ASSERT_NE (nullptr, ptrs[i]);
        }

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            memCtx.deallocate (ptrs[i]);
        }

        memCtx.releaseAll ();
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_EQ (false, memCtx.contains (ptrs[i]));
        }
    }

    {
        cookmem::MemContext<cookmem::CachedArena<cookmem::MallocArena>, cookmem::NoActionMemLogger> memCtx(cachedArena, logger);

        void* ptrs[NUM_ENTRIES];

        std::size_t size = 3;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            size *= 10;
            ptrs[i] = memCtx.allocate (size);
            ASSERT_EQ (true, memCtx.contains (ptrs[i]));
            ASSERT_NE (nullptr, ptrs[i]);
        }

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            memCtx.deallocate (ptrs[i]);
        }

        memCtx.releaseAll();
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_EQ (false, memCtx.contains (ptrs[i]));
        }
    }

    return 0;
}


int
main (int argc, const char* argv[])
{
    ASSERT_EQ (0, test1 ());

    return 0;
}
