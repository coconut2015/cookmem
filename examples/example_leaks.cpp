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
    // Padding stores the exact size of each allocation, so the report shows
    // the sizes that were requested.
    cookmem::SimpleMemContext<> memCtx (cookmem::MAX_PADDING_SIZE);

    void* ptr1 = memCtx.allocate (10);
    void* ptr2 = memCtx.allocate (2000);
    void* ptr3 = memCtx.allocate (30);
    memCtx.deallocate (ptr2);

    // Print the allocations that were not freed, and add up their sizes.
    std::size_t leakedBytes = 0;
    auto report = [&leakedBytes] (void* ptr, std::size_t size)
    {
        std::cout << "Not freed: " << ptr << ", " << size << " bytes" << std::endl;
        leakedBytes += size;
    };

    // Expect ptr1 and ptr3, then output 2 allocations, 40 bytes
    std::size_t leaks = memCtx.forEachAllocation (report);
    std::cout << leaks << " allocations, " << leakedBytes << " bytes" << std::endl;
    bool ok = (leaks == 2 && leakedBytes == 40);

    memCtx.deallocate (ptr1);
    memCtx.deallocate (ptr3);

    // Expect output 0 allocations, 0 bytes
    leakedBytes = 0;
    leaks = memCtx.forEachAllocation (report);
    std::cout << leaks << " allocations, " << leakedBytes << " bytes" << std::endl;
    ok = ok && leaks == 0;

    return ok ? 0 : 1;
}
