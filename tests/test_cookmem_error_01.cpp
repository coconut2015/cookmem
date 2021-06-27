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
testError1 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (nullptr, memCtx.allocate (0xffffffffffffffffUL));

    return 0;
}

static int
testError2 ()
{
    try
    {
        cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

        void* ptr = memCtx.allocate (1000);
        ASSERT_NE (nullptr, ptr);
        memCtx.deallocate (ptr);

        // double free
        memCtx.deallocate (ptr);
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_DOUBLE_FREE, ex.getError ());

        return 0;
    }
    catch (...)
    {
    }

    return 1;
}

static int
checkBound (int size)
{
    try
    {
        cookmem::SimpleMemContext<cookmem::MallocArena> memCtx (true);

        void* ptr = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr);
        memset (ptr, 0xff, size + 1);
        memCtx.deallocate (ptr);
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_PADDING, ex.getError ());

        return 0;
    }
    catch (...)
    {
    }
    return 1;
}

static int
testError3 ()
{
    for (int size = 30000; size < 30030; ++size)
    {
        if (checkBound (size) != 0)
        {
            return 1;
        }
    }
    return 0;
}

int
main (int argc, const char* argv[])
{
    ASSERT_EQ (0, testError1 ());
    ASSERT_EQ (0, testError2 ());
    ASSERT_EQ (0, testError3 ());
    return 0;
}
