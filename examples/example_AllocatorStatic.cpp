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

typedef cookmem::CachedMemContext<> MemoryContext;

// Here we create a unique class called MyContextTag.
// It does not have any other purpose than to be a unique type.
struct MyContextTag;
// Now we create a class MyContextProvider that has a single static instance of MemoryContext.
// This provider is used to provide a single context to all allocators that use it.
typedef cookmem::TLSContextProvider<MemoryContext, MyContextTag> MyContextProvider;
// Every allocator using MyContextProvider shares its single context.
typedef cookmem::Allocator<int, MyContextProvider> MyStaticAllocator;

int
main ()
{
    std::list<int, MyStaticAllocator> values;

    values.push_back (10);
    values.push_back (20);
    values.push_back (30);

    for (std::list<int, MyStaticAllocator>::const_iterator it = values.begin ();
         it != values.end ();
         ++it)
    {
        std::cout << *it << std::endl;
    }

    return 0;
}
