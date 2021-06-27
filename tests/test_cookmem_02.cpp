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

static int
test1 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (false, memCtx.isStoringExactSize ());

    ASSERT_EQ (0, memCtx.getSize (nullptr));

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (32, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);
    ASSERT_EQ (0, memCtx.getSize (ptr));

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (304, memCtx.getSize (ptr));
    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (3008, memCtx.getSize (ptr));
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (30000, memCtx.getSize (ptr));
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000, memCtx.getSize (ptr));
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000, memCtx.getSize (ptr));
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
    ASSERT_EQ (30, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 300);
    ASSERT_EQ (300, memCtx.getSize (ptr));

    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (3000, memCtx.getSize (ptr));

    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (30000, memCtx.getSize (ptr));

    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000, memCtx.getSize (ptr));
    memset (ptr, 0xff, 30000);
    ASSERT_NE (nullptr, ptr);

    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000, memCtx.getSize (ptr));
    memset (ptr, 0xff, 30000);
    ASSERT_NE (nullptr, ptr);

    return 0;
}

static int
test3 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx (true);

    ASSERT_EQ (true, memCtx.isPadding ());
    ASSERT_EQ (true, memCtx.isStoringExactSize ());

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 30);
    ASSERT_EQ (30, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 300);
    ASSERT_EQ (300, memCtx.getSize (ptr));

    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (3000, memCtx.getSize (ptr));
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

int
main (int argc, const char* argv[])
{
    ASSERT_EQ (0, test1 ());
    ASSERT_EQ (0, test2 ());
    ASSERT_EQ (0, test3 ());
    return 0;
}
