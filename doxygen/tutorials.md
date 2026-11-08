Tutorials {#tutorials}
=========

CookMem divides the job of a memory context into three parts.

1. The memory arena provides the API for allocating and deallocating large
   memory segments.
2. The memory pool manages memory chunks within memory segments. This
   component is typically managed and hidden by the memory context.
3. The logger monitors allocation and deallocation activity. Its primary use
   is to gather memory statistics.

Separating the memory arena from the memory pool makes it easy to build a
custom memory context for your needs. See \ref references "References" for
the available arenas and loggers. CookMem also provides ready-to-use
memory contexts, including cookmem::SimpleMemContext and
cookmem::CachedMemContext. This flexibility was a primary motivation for
the project.

\section FixedArena FixedArena

Often, we want allocations to be done within a large chunk of memory, such as a
stack-based or pre-allocated memory buffer.
cookmem::FixedArena is for this purpose.

\include example_FixedArena.cpp

\section MmapArena MmapArena

[mmap](https://en.wikipedia.org/wiki/Mmap) is one way to obtain large
segments of memory on Unix.  cookmem::MmapArena calls `mmap` and `munmap` to obtain and release segments.
On Windows, it uses `VirtualAlloc` and `VirtualFree` instead.

It should be noted that this arena does not track the memory segments
since cookmem::MemPool already does the tracking.

\include example_SimpleMemContext.cpp

\section CachedArena CachedArena

cookmem::CachedArena caches released memory segments in a
[red-black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree) and reuses them for
later requests.  This is useful when it is expensive to request and
release memory segments.

Note that the memory pool does not release a segment to the arena as soon as
the segment becomes free.  It keeps free segments for later allocations, and
releases them periodically, after a number of deallocations.

\include example_CachedArena.cpp

\section copying Copying Data Between Memory Pools

cookmem can determine whether a memory pointer is in the address space of a
memory pool. The check iterates through all memory segments owned by the pool.

This ability is quite useful to copy needed data out from the memory
pool that is about to be destroyed.

\include example_child.cpp

\section querying Querying Pointer Information

cookmem can report the allocated size of a pointer. By default, cookmem does
not keep track of the exact allocated size. However, an option can be enabled
to save the exact size. This option does not use extra memory, but it does
require additional bookkeeping time.

Additionally, cookmem can determine whether a pointer is valid.

See the following example.

\include example_getSize.cpp

\section footprint Restricting Memory Usage

A memory context can limit the total size of the memory segments it obtains,
which is its footprint.  An allocation that would need memory beyond the
limit fails and returns `nullptr`, while the context remains usable.  This
protects long-running programs from accidentally requesting a very large
amount of memory.

* `setFootprintLimit ()` sets the limit in bytes.  0, the default, means no
  limit.
* `getFootprint ()` returns the current footprint, and `getMaxFootprint ()`
  returns the largest footprint so far.

The footprint includes the overhead of the segments, so slightly less memory
than the limit is available for allocations.

\include example_footprintLimit.cpp

\section statistics Memory Statistics

cookmem::StatsLogger gathers statistics on allocations and memory segments:
counts, current and peak sizes, and how the allocations were satisfied.  Use
it as the logger of a memory context and read the statistics with
`getLogger ().getStats ()`.  The default logger, cookmem::NoActionMemLogger,
does nothing, so the statistics cost nothing unless you ask for them.

\include example_StatsLogger.cpp

\section boundcheck Memory Corruption Detection

In addition to various assertions, cookmem can add padding bytes (between 1
and 8) after the end of the user-allocated memory. When a pointer is explicitly
deallocated, a check determines whether the padding bytes were overwritten.

This feature can reduce allocation and deallocation performance. In some cases,
it can also increase memory usage. However, it is a valuable debugging tool.

Note that once an exception is thrown, the memory context is in a bad state, so
it is not a good idea to reuse it.

\include example_boundcheck.cpp

\section callerSTLAlloc Caller-owned STL Allocator

Standard library containers accept custom allocators, allowing their storage
to come from a CookMem memory context. cookmem::Allocator<T> adapts a context
to the C++ allocator interface. For example, `std::vector`, `std::list`, and
`std::basic_string` can allocate from the same context as the rest of the
application.

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
currently used for allocations. When no matching scope is active, it returns
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
