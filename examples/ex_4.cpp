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

int main(int argc, const char* argv[])
{
    cookmem::MmapArena arena;
    cookmem::CachedArena<cookmem::MmapArena> cachedArena (arena);

    cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena>> parentPool (cachedArena);

    int* ptrs[10];

    // allocate memory in the parent pool
    for (int i = 0; i < 5; ++i)
    {
        ptrs[i] = (int*)parentPool.allocate(sizeof(int));
        *ptrs[i] = i;
    }

    {
        cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena>> childPool (cachedArena);

        // allocate memory in the child pool
        for (int i = 5; i < 10; ++i)
        {
            ptrs[i] = (int*)childPool.allocate(sizeof(int));
            *ptrs[i] = i;
        }

        // before releasing the child pool, copy things to parent pool if necessary
        for (int i = 0; i < 10; ++i)
        {
            if (childPool.contains(ptrs[i]))
            {
                std::cout << "Copying index " << i << std::endl;
                int* ptr = (int*)parentPool.allocate (sizeof(int));
                *ptr = *ptrs[i];
                ptrs[i] = ptr;
            }
        }
    }

    // verify all memories are owned by parent pool
    for (int i = 0; i < 10; ++i)
    {
        if (!parentPool.contains(ptrs[i]))
        {
            std::cout << "Oops: index " << i << " is problematic." << std::endl;
            return 1;
        }
    }
    return 0;
}
