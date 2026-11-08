Limitations {#limitations}
===========

### No Thread Safety

A memory context is not thread-safe.  Use each context from one thread at a
time, for example one context per thread.  Locking support is not provided
and is unlikely to be implemented, since this project is intended for use
within a thread.  Consider [nedmalloc](https://github.com/ned14/nedmalloc)
instead if you need a thread-safe allocator.

### Limited Alignment

Every pointer returned by a memory context is aligned to
cookmem::ALLOCATION_ALIGNMENT, which is twice the size of `std::size_t`:
16 bytes on 64-bit platforms and 8 bytes on 32-bit platforms.

Types that require a larger alignment, such as types declared with
`alignas(32)`, are not supported.  cookmem::Allocator rejects them at compile
time.  Raw allocations from a memory context are not checked, so do not use
them for such types.

### Best-Effort Error Detection

The checks for memory errors are designed to be cheap, so they cannot catch
everything.

* Double free detection is best effort.  If the memory has already been
  reused by another allocation, the second free cannot be detected.
* Padding bytes only detect writes just past the end of an allocation, and
  only when the allocation is deallocated explicitly.
* Passing a pointer that was not allocated by the memory context to
  `deallocate` is not detected and is undefined behavior.
