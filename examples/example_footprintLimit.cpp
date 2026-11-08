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
#include <iostream>

#include <cookmem.h>

int
main (int argc, const char* argv[])
{
    cookmem::SimpleMemContext<> memCtx;

    // Limit the total size of the memory segments to 1 MB.  0, the default,
    // means no limit.
    memCtx.setFootprintLimit (1024 * 1024);

    // A request that can never fit within the limit fails right away.  The
    // context is not affected and can still be used.
    void* huge = memCtx.allocate (10 * 1024 * 1024);
    std::cout << "10 MB request: " << (huge == nullptr ? "failed" : "succeeded") << std::endl;

    // Allocate 100 KB blocks until the limit is reached.
    const int maxBlocks = 100;
    void* blocks[maxBlocks];
    int numBlocks = 0;
    while (numBlocks < maxBlocks)
    {
        void* ptr = memCtx.allocate (100 * 1024);
        if (ptr == nullptr)
        {
            break;
        }
        blocks[numBlocks++] = ptr;
    }

    std::cout << "100 KB blocks allocated: " << numBlocks << std::endl;
    std::cout << "Footprint: " << memCtx.getFootprint () << " bytes" << std::endl;

    // Freeing memory makes room again.
    memCtx.deallocate (blocks[--numBlocks]);
    void* ptr = memCtx.allocate (100 * 1024);
    std::cout << "After freeing one block, another 100 KB request: "
              << (ptr == nullptr ? "failed" : "succeeded") << std::endl;
    memCtx.deallocate (ptr);

    for (int i = 0; i < numBlocks; ++i)
    {
        memCtx.deallocate (blocks[i]);
    }

    // The largest footprint reached so far.
    std::cout << "Maximum footprint: " << memCtx.getMaxFootprint () << " bytes" << std::endl;

    return 0;
}
