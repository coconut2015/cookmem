Detailed Behavior {#behavior}
=================

This page describes how CookMem behaves in situations that are easy to
overlook.  See \ref limitations "Limitations" for what CookMem does not
support.

### Allocation Failures

`allocate` and `reallocate` of a memory context return `nullptr` when a
request cannot be satisfied, for example when it is too large, when the arena
cannot provide a segment, or when the footprint limit is reached.  When
`reallocate` fails, the original pointer remains valid.

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
