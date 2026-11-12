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
testError1 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (nullptr, memCtx.allocate (~static_cast<std::size_t> (0)));

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
        ASSERT_NE (nullptr, ex.getMessage ());
        ASSERT_NE (nullptr, ex.getFile ());
        ASSERT_NE (0, ex.getLine ());

        return 0;
    }
    catch (...)
    {
    }

    return 1;
}

/**
 * A double free is detected after the chunk was merged into the free chunk
 * before it, and after the free chunk after it was merged into it.
 */
template<class TreeType>
static int
testDoubleFreeAfterMerge (std::size_t size)
{
    cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::NoActionMemLogger, TreeType> memCtx;

    void* first = memCtx.allocate (size);
    void* second = memCtx.allocate (size);
    void* third = memCtx.allocate (size);
    void* fourth = memCtx.allocate (size);
    ASSERT_NE (nullptr, fourth);

    // second is merged into first.
    memCtx.deallocate (first);
    memCtx.deallocate (second);
    try
    {
        memCtx.deallocate (second);
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_DOUBLE_FREE, ex.getError ());
    }

    // third is merged into the free chunk before it.  Then fourth is merged
    // with the free chunks on both sides.
    memCtx.deallocate (third);
    memCtx.deallocate (fourth);
    try
    {
        memCtx.deallocate (third);
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_DOUBLE_FREE, ex.getError ());
    }
    try
    {
        memCtx.deallocate (fourth);
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_DOUBLE_FREE, ex.getError ());
    }

    memCtx.validate ();
    return 0;
}

static int
checkBound (int size)
{
    try
    {
        cookmem::SimpleMemContext<cookmem::MallocArena> memCtx (4);

        void* ptr = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr);
        memset (ptr, 0xff, size + 4);
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

static int
testInvalidPaddingSize ()
{
    try
    {
        cookmem::SimpleMemContext<cookmem::MallocArena> memCtx (9);
    }
    catch (cookmem::Exception& ex)
    {
        ASSERT_EQ (cookmem::MEM_ERROR_GENERAL, ex.getError ());
        return 0;
    }
    return 1;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, testError1 ());
        ASSERT_EQ (0, testError2 ());
        // Small chunks, and large chunks merged in a tree or in the dv.
        ASSERT_EQ (0, testDoubleFreeAfterMerge<cookmem::PtrRBTree> (100));
        ASSERT_EQ (0, testDoubleFreeAfterMerge<cookmem::PtrRBTree> (1000));
        ASSERT_EQ (0, testDoubleFreeAfterMerge<cookmem::PtrAVLTree> (1000));
        ASSERT_EQ (0, testError3 ());
        ASSERT_EQ (0, testInvalidPaddingSize ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    return 0;
}
