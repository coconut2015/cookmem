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
#include <thread>

#include <cookmem.h>

typedef cookmem::CachedMemContext<cookmem::MmapArena> MemoryContext;

// Here we create a unique class called MyContextTag.
// It does not have any other purpose than to be a unique type.
struct MyContextTag;
// The provider makes this context available to default-constructed allocators.
typedef cookmem::TLSContextProvider<MemoryContext, MyContextTag> MyContextProvider;
// Create an allocator for int type using the context provider.
// This allocator can be used to create STL containers without explicitly passing
// the allocator to the container constructor.
typedef cookmem::Allocator<int, MyContextProvider> MyAllocator;

void
printValues (const std::list<int, MyAllocator>& values)
{
    for (std::list<int, MyAllocator>::const_iterator it = values.begin ();
         it != values.end ();
         ++it)
    {
        std::cout << *it << std::endl;
    }
}

void
runWithThreadLocalContext ()
{
    // Without a TLSContextScope, the provider uses one context per thread.
    // The context releases its memory segments when this thread exits.
    std::list<int, MyAllocator> values;

    values.push_back (10);
    values.push_back (20);
    values.push_back (30);

    printValues (values);
}

void
runWithSharedArena ()
{
    // This scope supplies a local pool that shares the thread-local cached
    // arena. Its free segments can be reused by other contexts on this thread.
    cookmem::TLSContextScope<MemoryContext, MyContextTag> contextScope (true);
    std::list<int, MyAllocator> values;

    values.push_back (10);
    values.push_back (20);
    values.push_back (30);

    printValues (values);
}

void
runWithSeparateArena ()
{
    // This scope supplies a local pool with its own arena.
    cookmem::TLSContextScope<MemoryContext, MyContextTag> contextScope (false);
    std::list<int, MyAllocator> values;

    values.push_back (10);
    values.push_back (20);
    values.push_back (30);

    printValues (values);
}

void
runWorkerThread ()
{
    {
        std::list<int, MyAllocator> values;

        values.push_back (10);
        values.push_back (20);
        values.push_back (30);

        printValues (values);
    }   // Destroy the containers before releasing their memory.

    // Return this thread's memory to the system now, instead of waiting for
    // the thread-local context to be destroyed when the thread exits.  Do
    // this when the thread could end without running thread-local
    // destructors.  releaseAll () would instead keep the segments in the
    // cached arena, ready for later allocations on this thread.
    MyContextProvider::getThreadContext ().releaseAllToSystem ();
}

int
main ()
{
    runWithThreadLocalContext ();
    runWithSharedArena ();
    runWithSeparateArena ();

    std::thread worker (runWorkerThread);
    worker.join ();

    return 0;
}
