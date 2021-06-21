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
#include <cookmallocarena.h>

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

static int
test1 ()
{
    cookmem::MallocArena arena;
    cookmem::MemPool<cookmem::MallocArena> pool (arena);

    void* ptr;

    ptr = pool.allocate(30);
    ASSERT_NE(nullptr, ptr);
    pool.deallocate(ptr);

    ptr = pool.allocate(300);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(3000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);

    return 0;
}

static int
test2 ()
{
    cookmem::MallocArena arena;
    cookmem::MemPool<cookmem::MallocArena> pool (arena);

    void* ptr[10000];

    int size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = pool.allocate(size);
        ASSERT_NE(nullptr, ptr[i]);
    }

    for (int i = 0; i < 10000; ++i)
    {
        pool.deallocate(ptr[i]);
    }

    size = 8;
    for (int i = 0; i < 10000; ++i)
    {
        size += 8;
        ptr[i] = pool.allocate(size);
        ASSERT_NE(nullptr, ptr[i]);
    }

    for (int i = 0; i < 10000; ++i)
    {
        pool.deallocate(ptr[i]);
    }

    return 0;
}

static int
test3 ()
{
    cookmem::MallocArena arena;
    cookmem::MemPool<cookmem::MallocArena> pool (arena);

    void* ptr[300];

    int size = 300;
    for (int i = 0; i < 300; ++i)
    {
        size += 8;
        ptr[i] = pool.allocate(size);
        ASSERT_NE(nullptr, ptr[i]);
    }

    for (int i = 0; i < 300; ++i)
    {
        pool.deallocate(ptr[i]);
    }

    size = 300;
    for (int i = 0; i < 300; ++i)
    {
        size += 8;
        ptr[i] = pool.allocate(size);
        ASSERT_NE(nullptr, ptr[i]);
    }

    for (int i = 0; i < 300; ++i)
    {
        pool.deallocate(ptr[i]);
    }

    return 0;
}

static int
testError1 ()
{
    cookmem::MallocArena arena;
    cookmem::MemPool<cookmem::MallocArena> pool (arena);

    ASSERT_EQ(nullptr, pool.allocate(0xffffffffffffffffUL));

    return 0;
}

int main(int argc, const char* argv[])
{
    ASSERT_EQ(0, test1 ());
    ASSERT_EQ(0, test2 ());
    ASSERT_EQ(0, test3 ());

    ASSERT_EQ(0, testError1 ());
    return 0;
}
