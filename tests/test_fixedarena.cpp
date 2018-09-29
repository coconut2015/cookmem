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
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));
    cookmem::MemPool<cookmem::FixedArena> pool (arena);

    void* ptr;

    ptr = pool.allocate(30);
    ASSERT_NE(nullptr, ptr);
    pool.deallocate(ptr);

    ptr = pool.allocate(300);
    ASSERT_EQ(true, pool.contains(ptr));
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(3000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_EQ(nullptr, ptr);

    ASSERT_EQ(false, pool.contains((void*)&arena));

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
    cookmem::MemPool<cookmem::FixedArena> pool (arena);

    void* ptr;

    ptr = pool.allocate(30);
    ASSERT_NE(nullptr, ptr);
    pool.deallocate(ptr);

    ptr = pool.allocate(300);
    ASSERT_EQ(true, pool.contains(ptr));
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(3000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_NE(nullptr, ptr);
    ptr = pool.allocate(30000);
    ASSERT_EQ(nullptr, ptr);

    ASSERT_EQ(false, pool.contains((void*)&arena));

    return 0;
}

static int
test3 ()
{
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));

    std::size_t size = 3000;
    void* ptr = arena.getSegment(size);
    ASSERT_EQ(ptr, buffer);
    ASSERT_EQ(size, sizeof(buffer));
    ASSERT_EQ(nullptr, arena.getSegment(size));
    ASSERT_EQ(false, arena.freeSegment(ptr, size));
    ASSERT_EQ(true, arena.freeSegment(ptr, size));

    return 0;
}

int main(int argc, const char* argv[])
{
    ASSERT_EQ(0, test1 ());
    ASSERT_EQ(0, test2 ());
    ASSERT_EQ(0, test3 ());

    return 0;
}
