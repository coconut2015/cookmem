/*
 * Copyright (c) 2025-2026 Heng Yuan
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
#include <list>

#include <cookmem.h>

int
main ()
{
    typedef cookmem::CachedMemContext<> MemoryContext;
    typedef cookmem::Allocator<int> IntAllocator;

    // Create a memory context
    MemoryContext context;
    // Create an allocator for int type using the memory context
    IntAllocator allocator (context);
    // Create a list of int using the custom allocator
    std::list<int, IntAllocator> values (allocator);

    values.push_back (10);
    values.push_back (20);
    values.push_back (30);

    for (std::list<int, IntAllocator>::const_iterator it = values.begin ();
         it != values.end ();
         ++it)
    {
        std::cout << *it << std::endl;
    }

    return 0;
}
