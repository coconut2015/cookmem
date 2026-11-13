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
// This example needs C++17.
#include <iostream>
#include <map>
#include <memory_resource>
#include <string>
#include <vector>

#include <cookmem.h>

int
main (int argc, const char* argv[])
{
    cookmem::SimpleMemContext<> memCtx;

    // A std::pmr::memory_resource that allocates from memCtx.
    cookmem::MemResource<cookmem::SimpleMemContext<>> resource (memCtx);

    // Adds up the sizes of the allocations in memCtx.
    std::size_t bytes = 0;
    auto addSize = [&bytes] (void*, std::size_t size)
    {
        bytes += size;
    };

    {
        // The containers, and the strings inside the map, allocate from
        // memCtx.
        std::pmr::vector<int> numbers ({ 1, 2, 3 }, &resource);
        std::pmr::map<int, std::pmr::string> names (&resource);
        names[1] = "one";
        names[2] = "a string long enough to need memory of its own";

        // Expect output 3 numbers, 2 names
        std::cout << numbers.size () << " numbers, " << names.size () << " names" << std::endl;

        // Expect output 1
        std::cout << memCtx.contains (numbers.data (), true) << std::endl;

        // The allocations of the vector, the map nodes and the long string.
        // The numbers depend on the standard library.
        std::size_t count = memCtx.forEachAllocation (addSize);
        std::cout << count << " allocations in memCtx, " << bytes << " bytes" << std::endl;
    }

    // Everything was freed when the containers were destroyed.  Expect
    // output 0 allocations in memCtx, 0 bytes
    bytes = 0;
    std::size_t count = memCtx.forEachAllocation (addSize);
    std::cout << count << " allocations in memCtx, " << bytes << " bytes" << std::endl;

    return (count == 0 && bytes == 0) ? 0 : 1;
}
