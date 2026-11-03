References {#references}
==========

### Core Backend Components

#### Memory Arenas

Memory arena classes are used to obtain large chunk of memory.

These classes implement cookmem::MemArena interface without explicitly inheriting
from it.  This is to avoid the overhead of virtual function calls and allows inlining
of the functions.

* cookmem::FixedArena - Wrap around a large chunk of pre-allocated memory and use it as the segment.
* cookmem::MallocArena - Use `malloc` / `free` to get a large segment.
* cookmem::MmapArena - Use `mmap` / `munmap` on Unix and `VirtualAlloc` and `VirtualFree` on Windows to get a large segment.
* cookmem::CachedArena - Cache the segments released to be used later rather than discarding them immediately.

#### Memory Logger

Memory logger classes are used to obtain large chunk of memory.

These classes implement cookmem::MemLogger interface without explicitly
inheriting from it.  This is to avoid the overhead of virtual function
calls and allows inlining of the functions.

* cookmem::NoActionMemLogger - Do nothing.
* cookmem::StatsLogger - Mostly for debugging purposes to gather statistics on memory allocations.

#### Memory Pool

Memory pool does the allocation / deallocation of smaller chunks of memory inside the segments
obtained from memory arenas.

Currently, there is only a single implementation.

* cookmem::MemPool

### Memory Contexts

Memory context classes are contains the API interfaces users actually call to
allocate / deallocate memories.  They use the backend cookmem::MemArena,
cookmem::MemLogger and cookmem::MemPool to do the actual work.

The idea is that one can mix match different backend components to create
a suitable memory context that one needs.

* cookmem::SimpleMemContext - Default uses cookmem::MmapArena + cookmem::NoActionMemLogger
* cookmem::CachedMemContext - Default uses cookmem::CachedArena<cookmem::MmapArena> + cookmem::NoActionMemLogger
