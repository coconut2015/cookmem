References {#references}
==========

### Core Backend Components

#### Memory Arenas

Memory arena classes are used to obtain large chunks of memory. A memory arena
can be shared by multiple memory contexts.

These classes implement the cookmem::MemArena interface without explicitly
inheriting from it. This avoids the overhead of virtual function calls and
allows the functions to be inlined.

* cookmem::FixedArena - Wraps a large chunk of pre-allocated memory and uses it as the segment.
* cookmem::MallocArena - Uses `malloc` / `free` to get a large segment.
* cookmem::MmapArena - Uses `mmap` / `munmap` on Unix and `VirtualAlloc` / `VirtualFree` on Windows to get a large segment.
* cookmem::CachedArena - Caches released segments for later use rather than discarding them immediately.

#### Memory Logger

Memory logger classes are used to log memory context activities.

These classes implement the cookmem::MemLogger interface without explicitly
inheriting from it. This avoids the overhead of virtual function calls and
allows the functions to be inlined.

* cookmem::NoActionMemLogger - Do nothing.
* cookmem::StatsLogger - Mostly for debugging purposes to gather statistics on memory allocations.

#### Memory Pool

The memory pool allocates and deallocates smaller chunks of memory within the
segments obtained from memory arenas.

Currently, there is only a single implementation.

* cookmem::MemPool

### Memory Contexts

Memory context classes contain the API that users call to allocate and
deallocate memory. They use the backend cookmem::MemArena,
cookmem::MemLogger and cookmem::MemPool to do the actual work.

The backend components can be mixed and matched to create a suitable memory
context.

Furthermore, multiple memory contexts can share the same memory arena.
With cookmem::CachedArena, when one memory context is done, another context
can reuse the segments allocated by the previous context. Thus, we can further
avoid expensive calls to allocate large segments.

* cookmem::SimpleMemContext - Default uses cookmem::MmapArena + cookmem::NoActionMemLogger
* cookmem::CachedMemContext - Default uses cookmem::CachedArena<cookmem::MmapArena> + cookmem::NoActionMemLogger

### STL Allocator

There is one STL allocator implementation, cookmem::Allocator. It depends
on a memory context provider to provide the cookmem::MemContext instance.

* cookmem::Allocator

There are two memory context providers.

* cookmem::MemContextProvider - caller owned memory context
* cookmem::TLSContextProvider - thread-local storage memory context

The scoped context helper temporarily overrides the context used by
default-constructed allocators with the matching cookmem::TLSContextProvider.
Declare it before containers that use the scoped context so the containers
are destroyed before the scope ends.

* cookmem::TLSContextScope - scopes a context to a lexical block

### Polymorphic Memory Resource

With C++17, a memory context can also be used through
`std::pmr::memory_resource`, so that the `std::pmr` containers can allocate
from it.

* cookmem::MemResource - a `std::pmr::memory_resource` that allocates from a caller owned memory context
