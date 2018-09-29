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

    {
        cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena>> pool (cachedArena);

        // allocate memory
        void* ptr = pool.allocate(100);

        // change the size of the memory
        ptr = pool.reallocate(ptr, 1000);

        // free the memory
        pool.deallocate(ptr);

        // destructor of MemPool is called to release all allocated memory
        // segments back to cachedArena
    }

    {
        cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena>> pool (cachedArena);

        // allocate memory.  The memory segment saved in cachedArena are re-used.
        void* ptr = pool.allocate(100);

        // change the size of the memory
        ptr = pool.reallocate(ptr, 1000);

        // free the memory
        pool.deallocate(ptr);

        // destructor of MemPool is called to release all allocated memory
        // segments back to cachedArena
    }
    return 0;
}
