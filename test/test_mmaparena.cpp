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

static int
test1 ()
{
    cookmem::MmapArena arena;
    cookmem::MemPool<cookmem::MmapArena> pool (arena);

    void* ptrs[10];

    std::size_t size = 3;
    for (int i = 0; i < 10; ++i)
    {
        size *= 10;
        ptrs[i] = pool.allocate(size);
        ASSERT_EQ(true, pool.contains (ptrs[i]));
        ASSERT_NE(nullptr, ptrs[i]);
    }

    for (int i = 0; i < 10; ++i)
    {
        pool.deallocate(ptrs[i]);
    }

    pool.releaseAll();
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_EQ(false, pool.contains (ptrs[i]));
    }

    ASSERT_EQ(nullptr, pool.allocate(0xffffffffff000000));

    return 0;
}

static int
test2 ()
{
    cookmem::MmapArena arena;
    cookmem::MemPool<cookmem::MmapArena> pool (arena);

    std::size_t size = 300;
    void* ptr = pool.callocate(1, size);
    for (std::size_t s = 0; s < size; ++s)
    {
        ASSERT_EQ(0, ((char*)ptr)[s]);
    }
    pool.deallocate(ptr);

    ptr = pool.callocate(10, size);
    for (std::size_t s = 0; s < 10 * size; ++s)
    {
        ASSERT_EQ(0, ((char*)ptr)[s]);
    }
    pool.deallocate(ptr);

    return 0;
}

static int
test3 ()
{
    cookmem::MmapArena arena;
    cookmem::MemPool<cookmem::MmapArena> pool (arena);

    std::size_t size = 300;
    void* ptr = pool.callocate(1, size);
    for (std::size_t s = 0; s < size; ++s)
    {
        ASSERT_EQ(0, ((char*)ptr)[s]);
    }

    ptr = pool.reallocate(ptr, 10 * size);
    ASSERT_NE(nullptr, ptr);
    pool.deallocate(ptr);

    ptr = pool.reallocate(nullptr, 10 * size);
    ASSERT_NE(nullptr, ptr);

    void* ptr2 = pool.reallocate(ptr, size);
    ASSERT_EQ(ptr, ptr2);
    pool.deallocate(ptr);

    return 0;
}

static int
test4 ()
{
    cookmem::MmapArena arena;
    cookmem::MemPool<cookmem::MmapArena> pool (arena);

    pool.setFootprintLimit(1000000);

    void* ptr = pool.allocate (800000);
    ASSERT_NE(nullptr, ptr);

    ASSERT_EQ(nullptr, pool.allocate (800000));
    return 0;
}


int main(int argc, const char* argv[])
{
    ASSERT_EQ(0, test1 ());
    ASSERT_EQ(0, test2 ());
    ASSERT_EQ(0, test3 ());
    ASSERT_EQ(0, test4 ());

    return 0;
}
