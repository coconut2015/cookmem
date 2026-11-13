/*
 * Copyright (c) 2026 Heng Yuan
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
// Compiled with every warning enabled and warnings treated as errors (see
// CMakeLists.txt), so that the CookMem headers stay free of warnings in the
// programs that include them.  It instantiates both trees, the contexts,
// the loggers, padding and the STL allocator.
#include <vector>

#include <cookmem.h>
#include "unittest.h"

typedef cookmem::SimpleMemContext<cookmem::MmapArena, cookmem::StatsLogger, cookmem::PtrAVLTree> AvlContext;

static int
test1 ()
{
    cookmem::SimpleMemContext<> rbCtx (cookmem::MAX_PADDING_SIZE);
    AvlContext avlCtx;
    cookmem::CachedMemContext<> cachedCtx;

    std::vector<void*> rbPtrs;
    std::vector<void*> avlPtrs;
    for (int i = 0; i < 2000; ++i)
    {
        rbPtrs.push_back (rbCtx.allocate (static_cast<std::size_t> (16 + i * 7)));
        avlPtrs.push_back (avlCtx.allocate (static_cast<std::size_t> (300 + i * 13)));
        cachedCtx.deallocate (cachedCtx.allocate (100));
    }
    void* aligned = rbCtx.allocateAligned (100, 64);
    ASSERT_NE (nullptr, aligned);
    ASSERT_EQ (true, rbCtx.resizeInPlace (aligned, 50));
    rbCtx.deallocate (aligned);

    for (std::size_t i = 0; i < rbPtrs.size (); i += 2)
    {
        rbCtx.deallocate (rbPtrs[i]);
        avlCtx.deallocate (avlPtrs[i]);
    }
    rbCtx.validate ();
    avlCtx.validate ();

    cookmem::Allocator<int> allocator (cachedCtx);
    std::vector<int, cookmem::Allocator<int>> values (allocator);
    values.push_back (1);

    const AvlContext& constCtx = avlCtx;
    ASSERT_EQ (std::size_t (2000), constCtx.getLogger ().getStats ().getNumAllocations ());
    return 0;
}

int
main ()
{
    try
    {
        ASSERT_EQ (0, test1 ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    return 0;
}
