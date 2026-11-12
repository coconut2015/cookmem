Detailed Behavior {#behavior}
=================

This page describes how CookMem behaves in situations that are easy to
overlook.  See \ref limitations "Limitations" for what CookMem does not
support.

### Allocation Failures

`allocate`, `allocateAligned` and `reallocate` of a memory context return
`nullptr` when a request cannot be satisfied, for example when it is too
large, when the arena cannot provide a segment, or when the footprint limit
is reached.  `allocateAligned` also returns `nullptr` when the alignment is
not a power of 2.  When `reallocate` fails, the original pointer remains
valid.

cookmem::Allocator throws `std::bad_alloc` instead, as the standard library
expects.

### Errors and Exceptions

* `deallocate` throws cookmem::Exception when it detects a double free
  (cookmem::MEM_ERROR_DOUBLE_FREE) or overwritten padding bytes
  (cookmem::MEM_ERROR_PADDING).  These checks are done in release builds too.
* Destructors are `noexcept` by default, so such an error detected while a
  container is being destroyed ends the program with `std::terminate`.
* Assertions (cookmem::MEM_ERROR_ASSERT) are checked only when `COOKMEM_DEBUG`
  is defined to a non-zero value.  CookMem's own Debug builds of its tests
  and examples define it; your code needs to define it to enable the checks.
* Once an exception is thrown, the memory context is in a bad state.  Do not
  use it again.

### Pointer Ownership

A pointer must be deallocated by the memory context that allocated it.  Use
`contains ()` to check whether a pointer is in the memory segments of a
context, for example before copying data out of a context that is about to
be destroyed.

### Releasing Memory

When memory is deallocated, the memory pool does not release memory segments
to the arena right away, even when a segment becomes entirely free.  Free
segments are kept for later allocations.  After every
cookmem::DEFAULT_TRIM_EMPTY_SEGMENTS_INTERVAL (4095) deallocations, the pool
runs a trimming pass that releases the segments that are entirely free to the
arena, which reduces the footprint.  As a result:

* The footprint can stay higher than the memory in use until the next
  trimming pass.
* With cookmem::CachedArena, the released segments are cached by the arena
  and reused for later requests, including requests from other memory
  contexts that share the arena.
* `setTrimEmptySegmentsInterval ()` changes the number of deallocations
  between trimming passes.  The default suits most programs.  A smaller
  interval keeps the footprint smaller, but releases and obtains segments
  more often.
* cookmem::StatsLogger counts the trimming passes
  (`getNumTrimEmptySegments ()`).

`releaseAll ()` releases all memory segments of a context at once.  Every
pointer allocated from the context becomes invalid.

The segments go to the arena of the context.  cookmem::CachedArena keeps them
for reuse, so the memory is not returned to the system yet.  This makes
`releaseAll ()` a cheap way to reset a context that is used again.
`releaseAllToSystem ()` also calls `releaseAll ()` on the arena, which
returns the cached segments to the underlying arena, for example before a
thread exits or when the program no longer needs the memory.  It empties the
cache even when other contexts share the arena.  Destroying a
cookmem::CachedArena also returns its cached segments.  Other arenas keep no
segments, so their `releaseAll ()` does nothing.

Destroying a memory context releases all of its memory segments at once,
without deallocating each allocation.  Destructors of objects still in that
memory are not called, so destroy any objects that own other resources (such
as files or memory from other allocators) before the context ends.

cookmem::FixedArena has a fixed capacity.  When it is used up, allocations
fail and return `nullptr`.

### Thread-Local Allocators

Containers that use cookmem::TLSContextProvider must stay on the thread where
their allocator was created.  They must also be destroyed before their
context ends: when the thread exits, or when the cookmem::TLSContextScope
ends.

When a thread exits normally, the thread-local context is destroyed and its
memory is returned to the system.  If a thread could end without running
thread-local destructors, release the memory before that point:

    MyContextProvider::getThreadContext ().releaseAllToSystem ();
