Tutorials {#tutorials}
=========

CookMem divides the job of a memory context into three parts.

1. Memory arena, which is responsible for providing the API to allocate
   / deallocate large memory segments.
2. Memory pool, which is responsible for managing memory chunks created
   within memory segments.  This component is typically managed and
   hidden by the memory context.
3. Logger, which can be used to monitor the allocation / deallocation
   activities.  The primary use of logger is to gather memory statistics.

Separating the memory arena from the memory pool makes it easy to build a
custom memory context for your needs. See [References](references.md) for
the available arenas and loggers. CookMem also provides ready-to-use
memory contexts, including cookmem::SimpleMemContext and
cookmem::CachedMemContext. This flexibility was a primary motivation for
the project.

\section FixedArena FixedArena

Oftentimes, we want the allocations to be done within a big chunk of
memory, which can be on stack, pre-allocated memory buffer etc.
cookmem::FixedArena is for this purpose.

\include example_FixedArena.cpp

\section MmapArena MmapArena

[mmap](https://en.wikipedia.org/wiki/Mmap) is one way to obtain large
segments of memory on Unix.  cookmem::MmapArena basically calls ```mmap```
and ```munmap``` to get and release segments.  On Windows, it uses ```VirtualAlloc``` / ```VirtualFree``` instead.

It should be noted that this arena does not track the memory segments
since cookmem::MemPool already does the tracking.

\include example_SimpleMemContext.cpp

\section CachedArena CachedArena

cookmem::CachedArena basically caches the memory segments released in an
[red-black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree) and reuses them for
later requests.  This is useful when it is expensive to request and
release memory segments.

\include example_CachedArena.cpp

\section copying Copying Data Between Memory Pools

cookmem has the ability to determine if a memory pointer is in the
address space of that memory pool.  Note that the check is done
by iterating through all the memory segments owned by the memory pool.

This ability is quite useful to copy needed data out from the memory
pool that is about to be destroyed.

\include example_child.cpp

\section querying Querying Pointer Information

cookmem has the ability to get the allocated size information for a
pointer.  By default, cookmem does not keep track the exact allocated
size.  However, it is possible to turn on an option to save the exact
allocated size.  This option does not cost extra memory, but it does
take extra time in bookkeeping.

Additionally, cookmem has the ability to tell if a pointer is valid.

See the following example.

\include example_getSize.cpp

\section boundcheck Memory Corruption Detection

Besides various assertions, cookmem has an ability to pad magic bytes
(between 1 and 8 bytes) after the end of the user allocated memory.
When a pointer is explicitly deallocated, a check is performed to see
if the padding bytes were overwritten.

This feature obviously costs performances in memory allocation and
deallocation.  In some cases can also increase the memory required.
However, it is a valuable debugging tool as well.

Note that once the exception is thrown, the memory context is in
a bad state.  So it is not good idea to re-use the same memory context.

\include example_boundcheck.cpp

\section callerSTLAlloc Caller-owned STL Allocator

The standard library containers accept custom allocators, allowing their
storage to come from a CookMem memory context. cookmem::Allocator<T> adapts
a context to the C++ allocator interface. For example, a ```std::vector```, ```std::list```,
or ```std::basic_string``` can allocate from the same context
as the rest of the application.

With the default provider, the application creates and owns the context, then
passes an allocator bound to it to each container. Containers using allocators
bound to the same context can share its memory pool. This makes context
ownership explicit and avoids global state, at the cost of passing allocator
instances when constructing containers.

\include example_Allocator.cpp

\section tlsSTLAlloc Thread-local and Scoped Context STL Allocators

cookmem::TLSContextProvider provides one context per thread for each context
type and tag type. Use a unique tag to keep allocator groups separate. By
default, this context lasts for the lifetime of the thread. To limit it to a
lexical scope instead, declare a cookmem::TLSContextScope before the
containers. Default-constructed allocators created inside that scope use its
context, which is destroyed when the scope exits.

Use `TLSContextProvider<Context, Tag>::getActiveContext()` to query the context
currently used for allocations; when no matching scope is active, it returns
the persistent thread-local context.
`TLSContextProvider<Context, Tag>::getThreadContext()` returns the persistent
thread-local context whether or not a scope is active. The provider instance's
`get()` still returns the context to which that instance was bound.

The function-scope example uses TLSContextScope to release the context as soon
as its containers are destroyed. It demonstrates both options: passing `true`
makes its pool share the thread-local context's arena; with CachedMemContext,
freed segments can be reused by either context. Passing `false` gives the scope
its own arena. The thread-local example omits the scope, so the provider uses
its per-thread context and releases it when the thread exits. In all cases,
containers must be destroyed before their context ends and remain on the
thread where their allocator was created. Neither approach requires passing
allocator instances to containers.

\include example_AllocatorThreadLocal.cpp
