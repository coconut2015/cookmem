Limitations {#limitations}
===========

### No Thread Safety

A memory context is not thread-safe.  Use each context from one thread at a
time, for example one context per thread.  Locking support is not provided
and is unlikely to be implemented, since this project is intended for use
within a thread.  Consider [nedmalloc](https://github.com/ned14/nedmalloc)
instead if you need a thread-safe allocator.

### Alignment

Every pointer returned by `allocate ()` of a memory context is aligned to
cookmem::ALLOCATION_ALIGNMENT, which is twice the size of `std::size_t`:
16 bytes on 64-bit platforms and 8 bytes on 32-bit platforms.

For a larger alignment, use `allocateAligned (size, alignment)`.  The
alignment must be a power of 2.  The memory is freed with `deallocate ()`.
`reallocate ()` keeps the memory in place when it shrinks, but if it has to
move the memory, the new memory has only the normal alignment.

To find an aligned address, the pool takes a chunk that is larger than the
request by the alignment, then returns the unused memory before and after
the result to the pool.  A large alignment therefore needs that much free
space, and may fail under a footprint limit that a normal allocation of the
same size would fit in.

cookmem::Allocator uses `allocateAligned ()` automatically for types that
require a larger alignment, such as types declared with `alignas(64)`, and
so does cookmem::MemResource for requests with a larger alignment.

### Best-Effort Error Detection

The checks for memory errors are designed to be cheap, so they cannot catch
everything.

* Double free detection is best effort.  If the memory has already been
  reused by another allocation, the second free cannot be detected.
* Padding bytes only detect writes just past the end of an allocation, and
  only when the allocation is deallocated explicitly.
* Passing a pointer that was not allocated by the memory context to
  `deallocate` is not detected and is undefined behavior.
