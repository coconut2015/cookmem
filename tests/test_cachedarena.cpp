/*
 * Copyright (c) 2018 Heng Yuan
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
#include <cookmmaparena.h>

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

#define NUM_ENTRIES 9

static int
test1 ()
{
    cookmem::MmapArena arena;
    cookmem::CachedArena<cookmem::MmapArena> cachedArena (arena);

    {
        cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena> > pool (cachedArena);

        void* ptrs[NUM_ENTRIES];

        std::size_t size = 3;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            size *= 10;
            ptrs[i] = pool.allocate(size);
            ASSERT_EQ(true, pool.contains (ptrs[i]));
            ASSERT_NE(nullptr, ptrs[i]);
        }

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            pool.deallocate(ptrs[i]);
        }

        pool.releaseAll();
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_EQ(false, pool.contains (ptrs[i]));
        }
    }

    {
        cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena> > pool (cachedArena);

        void* ptrs[NUM_ENTRIES];

        std::size_t size = 3;
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            size *= 10;
            ptrs[i] = pool.allocate(size);
            ASSERT_EQ(true, pool.contains (ptrs[i]));
            ASSERT_NE(nullptr, ptrs[i]);
        }

        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            pool.deallocate(ptrs[i]);
        }

        pool.releaseAll();
        for (int i = 0; i < NUM_ENTRIES; ++i)
        {
            ASSERT_EQ(false, pool.contains (ptrs[i]));
        }
    }

    return 0;
}


int main(int argc, const char* argv[])
{
    ASSERT_EQ(0, test1 ());

    return 0;
}
