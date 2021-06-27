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

int
main (int argc, const char* argv[])
{
    typedef cookmem::CachedMemContext<> ParentCachedMemCtx;
    typedef cookmem::MemContext<cookmem::CachedArena<cookmem::MmapArena>, cookmem::NoActionMemLogger>   ChildCachedMemCtx;

    // Create the parent ctx
    ParentCachedMemCtx parentCtx;

    int* ptrs[10];

    // Allocate memory in the parent ctx
    for (int i = 0; i < 5; ++i)
    {
        ptrs[i] = (int*)parentCtx.allocate (sizeof(int));
        *ptrs[i] = i;
    }

    {
        // Create the child ctx
        //
        // Note that the child shares the cachedArena with the parent
        ChildCachedMemCtx childCtx (parentCtx);

        // Allocate memory in the child ctx
        for (int i = 5; i < 10; ++i)
        {
            ptrs[i] = (int*)childCtx.allocate (sizeof(int));
            *ptrs[i] = i;
        }

        // Before releasing the child ctx, copy things to parent ctx if necessary
        for (int i = 0; i < 10; ++i)
        {
            if (childCtx.contains (ptrs[i]))
            {
                std::cout << "Copying index " << i << std::endl;
                int* ptr = (int*)parentCtx.allocate (sizeof(int));
                *ptr = *ptrs[i];
                ptrs[i] = ptr;
            }
        }

        // Child ctx is destroyed.  No individual pieces of memories are
        // deallocated.  Instead, all the segments child ctx uses are simply
        // released to the cachedArena, which can then be re-used by the
        // parent ctx.
        //
        // This approach is very efficient if when you need to deal with
        // lots of temporary small allocations that need to be deallocated
        // at end of the task.
        //
        // At the same time, you can avoid having potentially large memory
        // segments being allocated / deallocated frequently.
    }

    // Verify all memories are owned by parent ctx
    for (int i = 0; i < 10; ++i)
    {
        if (!parentCtx.contains (ptrs[i]))
        {
            std::cout << "Oops: index " << i << " is problematic." << std::endl;
            return 1;
        }
    }
    return 0;
}
