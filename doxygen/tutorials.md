Tutorials {#tutorials}
=========

CookMem divides the job of a memory context into two parts.

1. Memory arena, which is responsible for allocating large memory segments.
2. Memory pool, which is responsible for managing memory chunks created within
   memory segments.

The intention is that by using different memory arenas with memory pool, we
can deal with many different use cases.  It was the primary motivation for
this project.

\section ex_1 Simple Memory Wrapper Arena

Oftentimes, we want to allocations to be done within a big chunk of
memory, which can be on stack, pre-allocated memory buffer etc.
cookmem::FixedArena is for this purpose.

\include ex_1.cpp

\section ex_2 mmap Based Memory Arena

[mmap](https://en.wikipedia.org/wiki/Mmap) is one way to obtain large
segments of memory on Unix.  cookmem::MmapArena basically calls mmap
and munmap to get and release segments.

It should be noted that this arena does not track the memory segments
since cookmem::MemPool already does the tracking.

\include ex_2.cpp

\section ex_3 Cached Memory Arena

cookmem::CachedArena basically caches the memory segments released in an
AVL tree and reuses them for later requests.  This is useful when it is
expensive to get and release memory segments.

\include ex_3.cpp

\section ex_4 Copying Data Between Memory Pools

cookmem::MemPool has the ability to determine if a memory pointer is
in the address space of that memory pool.  Note that the check is done
by iterating through all the memory segments owned by the memory pool.

This ability is quite useful to copy needed data out from the memory
pool that is about to be destroyed.

\include ex_4.cpp
