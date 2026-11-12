Implementation Details {#implementation}
======================

This page describes how CookMem manages memory internally, and how it
compares with [dlmalloc](https://gee.cs.oswego.edu/dl/html/malloc.html),
which it is based on.  It is meant for readers who want to understand,
debug or modify the allocator.  You do not need it to use CookMem.

Sizes below are given for 64-bit platforms, with 32-bit values in
parentheses where they differ.

### Overview

CookMem is split into layers that are combined through templates:

* An **arena** provides large memory segments: cookmem::MmapArena (mmap or
  VirtualAlloc), cookmem::MallocArena (malloc), cookmem::FixedArena (one
  caller-provided buffer) and cookmem::CachedArena (caches freed segments of
  another arena).
* A **memory pool** (cookmem::MemPool) carves segments into chunks and
  implements the dlmalloc algorithms.  It is internal and is used through a
  memory context.
* A **memory context** (cookmem::MemContext, cookmem::SimpleMemContext,
  cookmem::CachedMemContext) owns or references an arena and a logger, and
  exposes the pool to users.
* A **logger** (cookmem::NoActionMemLogger, cookmem::StatsLogger) receives
  every allocation, deallocation, segment and error event.
* cookmem::Allocator adapts a context to the STL allocator interface.

Arenas and loggers are template parameters, not virtual interfaces, so calls
to them can be inlined.  With cookmem::NoActionMemLogger, logging costs
nothing.

### Chunks

Every allocation is a **chunk**.  A chunk starts with a header of two words
(16 bytes, or 8 bytes on 32-bit platforms):

| Field            | Meaning                                                  |
|------------------|----------------------------------------------------------|
| `m_prevFootSize` | Size of the previous chunk, with its used bit.           |
| `m_size`         | Size of this chunk, with its used bit and other flags.   |

The user pointer is just after the header.  Chunk sizes are multiples of
cookmem::ALLOCATION_ALIGNMENT (16, or 8), so the low bits of both fields are
free for flags:

* `BIT_USED` (1): the chunk is in use.
* `BIT_NOTEXACTSIZE` (2): the exact user size is stored in the last byte of
  the chunk (see \ref impl_user_size "User Size").

A chunk of size `S` that holds a request of `n` bytes satisfies
`S = round_up (n + 16, ALLOCATION_ALIGNMENT)`.  The smallest chunk is 32 bytes
(16 bytes), which is enough to hold the header and the two links of a free
list.

**Boundary tags.**  Like dlmalloc, CookMem keeps the size of each chunk both
at its start and at the start of the next chunk, so that a chunk can find
both of its neighbors in constant time.  Unlike dlmalloc, the footer is
written for used chunks as well as free ones, and it carries the used bit of
the previous chunk:

* In dlmalloc, `prev_foot` is only valid when the previous chunk is free.
  When it is in use, its user data overlaps `prev_foot`, and a `PINUSE` bit
  in the head of the next chunk says so.  The overhead of a used chunk is one
  word.
* In CookMem, `m_prevFootSize` is always valid and never overlaps user data.
  The overhead of a used chunk is two words (16 bytes, or 8 bytes).

The extra word buys simpler code and stronger checking: the double free
check, the debug consistency checks and validate () all rely on the footer
always matching the header.

### Segments

The pool obtains memory from the arena in **segments**, and keeps them in a
singly linked list.  A segment looks like this:

    +--------+--------+-------------+--------------------+-------------+
    | m_size | m_next | m_pad       | chunks ...         | fence chunk |
    +--------+--------+-------------+--------------------+-------------+
                      ^ first chunk header

* `m_pad` is the `m_prevFootSize` of the first chunk.  It is set to "used,
  size 0", so the first chunk never tries to merge with memory before it.
* The fence chunk at the end has `m_size` set to "used", so the last chunk
  never tries to merge with memory after it.

A new segment is requested when no free chunk can satisfy a request.  The
pool asks for the chunk size plus the segment overhead, and the arena may
round that up, for example to its minimum segment size
(cookmem::DEFAULT_MIN_SEGMENT_SIZE, 64 KB).  The requested chunk is split off
the start of the segment, and the rest becomes a free chunk.

Segments must be aligned to cookmem::ALLOCATION_ALIGNMENT.  The header before
the first user pointer is four words, so an aligned segment gives aligned
user pointers, and chunk sizes keep every later pointer aligned.  mmap and
VirtualAlloc return page-aligned memory, `malloc` returns memory aligned to
at least two words, and cookmem::FixedArena advances a misaligned buffer to
the next aligned address.

Adjacent segments are never merged, even if the arena happens to return
contiguous memory.

### Bins

Free chunks are kept in bins, as in dlmalloc:

* **Small bins.**  32 bins, 8 bytes apart, hold free chunks smaller than
  256 bytes.  Each bin is a circular doubly linked list of chunks of exactly
  one size, and a 32-bit map (`m_smallMap`) records which bins are not empty.
  Since chunk sizes are multiples of 16 on 64-bit platforms, only every other
  small bin is used there.  On 32-bit platforms all of them are used.
* **Tree bins.**  32 bins hold free chunks of 256 bytes or more.  Bin `2k`
  holds sizes from `2^(k+8)` up to `1.5 * 2^(k+8)`, and bin `2k+1` holds sizes
  from `1.5 * 2^(k+8)` up to `2^(k+9)`.  The last bin also holds everything
  larger, so it starts at 12 MB.  The bin index is computed from the two most
  significant bits of the size, using a bit-scan instruction where one is
  available, exactly as in dlmalloc.  A 32-bit map (`m_treeMap`) records
  which bins are not empty.

Within a tree bin, dlmalloc uses a bitwise trie.  CookMem uses a balanced
binary search tree keyed by size instead: a red-black tree
(cookmem::PtrRBTree, the default) or an AVL tree (cookmem::PtrAVLTree),
chosen with the `TreeType` template parameter.  Free chunks of the same size
share one tree node, and the others are kept in a doubly linked list
attached to it, as in dlmalloc.  Taking a chunk from that list does not
rebalance the tree.

The tree nodes, like the list links of small chunks, are stored inside the
free chunks themselves, so the bins need no memory of their own.  The second
word of a node is the size field of the chunk, so a node takes 7 words,
which fits in the smallest large chunk.

### The Designated Victim

dlmalloc keeps one free chunk aside as the **designated victim** (dv): the
chunk used for small requests that have no exact fit, so that consecutive
small allocations are carved from one place.  CookMem keeps a dv too, with
some differences:

* When a large free chunk is added to the bins and there is no dv, the chunk
  becomes the dv instead of going into a tree.  This applies both to freed
  chunks and to remainders left after a split.
* When the dv is split and the remainder is still a large chunk, the
  remainder stays the dv.  If the remainder is small, it goes into a small
  bin and there is no dv until a large chunk is freed or split off again.  In
  dlmalloc, a small remainder can stay the dv.
* When a chunk right after the dv is freed, the dv grows in place.

### Allocation

`allocate (size)` adds the padding, if any, rounds the request to a chunk
size, and looks for a chunk in this order:

For a small request (chunk smaller than 256 bytes):

1. **Exact fit.**  If the small bin for the size, or the next one, has a
   chunk, it is used whole.  A chunk from the next bin is only 8 bytes
   larger, which is too little to split off.
2. **Larger small bin.**  If the dv is too small for the request but a larger
   small bin is not empty, the smallest such chunk is taken, found with the
   bit map, and split.  The remainder goes to a bin.
3. **Designated victim.**  If the dv is large enough, it is split.
4. **Tree bins**, as for a large request below.
5. **New segment** from the arena.

For a large request:

1. **Tree bins.**  The best fit: starting with the bin for the size, the
   smallest chunk that is large enough is taken from the first bin that has
   one, and split.  As in dlmalloc, if the dv is also large enough, the tree
   chunk is only taken if it is smaller than the dv.  The search stops at the
   first bin whose chunks are all at least the size of the dv, and the tree
   search leaves a chunk in place if it is not smaller.
2. **Designated victim**, if it is large enough.
3. **New segment** from the arena.

A split leaves a remainder only if it is at least the minimum chunk size;
otherwise the whole chunk is used.  Requests too large to represent
(`MAX_REQUEST`, as in dlmalloc) fail immediately.  A request that would push
the footprint over the limit set with `setFootprintLimit ()` fails instead of
obtaining a new segment, like `malloc_set_footprint_limit` in dlmalloc.

When an allocation fails, the logger is told and `nullptr` is returned.

### Aligned Allocation

`allocateAligned (size, alignment)` uses the same approach as dlmalloc's
`memalign`:

1. Allocate a chunk large enough for the request plus the alignment plus one
   minimum chunk.
2. Find the first address in it, after the header, that is aligned to the
   alignment and leaves at least a minimum chunk in front of it.  If the user
   pointer is already aligned, nothing is skipped.
3. Free the chunk in front of the aligned address.  It does not need to be
   merged with its previous neighbor, since free chunks are never adjacent,
   so that neighbor is in use.
4. Free the memory after the requested size, merged with the next chunk if
   that one is free.

The result is an ordinary chunk, so `deallocate ()`, `getSize ()`, exact
sizes and padding all work as usual.  cookmem::Allocator calls
`allocateAligned ()` for types whose alignment is larger than
cookmem::ALLOCATION_ALIGNMENT.

### Deallocation and Coalescing

`deallocate ()` merges the freed chunk with its free neighbors immediately,
using the boundary tags, so no two free chunks are ever next to each other.
The merged chunk goes back to a bin.

There is one fast path: when the previous chunk is free and the next one is
in use, the previous chunk may be able to absorb the freed chunk without
leaving its bin.  This is always possible for the dv.  For a chunk in a tree,
it is possible when the new size stays in the same bin and the same place in
the tree.  This is common when memory is freed in the order it was
allocated.

### Reallocation

* **Shrinking**, or growing within the same chunk, happens in place.  The
  new size, including the padding, is compared with the actual chunk size.
  If the unused tail is at least a minimum chunk, it is split off, merged
  with the next chunk if that one is free, and returned to a bin.  The exact
  size and the padding bytes are written again for the new size.
* **Growing** beyond the chunk happens in place if the next chunk is free
  and the two chunks together are large enough, as in dlmalloc.  The next
  chunk is taken out of its bin or is the dv, and the unused tail, if it is
  at least a minimum chunk, goes back to a bin.  The chunk after the next one
  is in use, so the tail does not need merging.
* Otherwise, growing allocates new memory, copies the data and frees the old
  memory.
* A new size too large to represent fails, and the old memory is unchanged.

`callocate (num, size)` fails if `num * size` overflows, as `calloc` does in
dlmalloc.

### Returning Memory to the Arena

* Every cookmem::DEFAULT_TRIM_EMPTY_SEGMENTS_INTERVAL (4095) deallocations,
  the pool releases every segment that has become entirely free.  This is
  the same idea as dlmalloc's `release_unused_segments`, which runs on a
  countdown of 4095 frees (`MAX_RELEASE_CHECK_RATE`).  The interval can be changed with
  `setTrimEmptySegmentsInterval ()`.
* When an allocation of at least
  cookmem::DEFAULT_IMMEDIATE_RELEASE_THRESHOLD (256 KB) is freed and its
  segment becomes entirely free, the segment is released right away instead
  of at the next trimming pass.  A large allocation usually gets a segment of
  its own, so this is similar to how dlmalloc unmaps requests above its mmap
  threshold.  The threshold can be changed, or set to 0 to disable this, with
  `setImmediateReleaseThreshold ()`.  The segment is found from its first
  chunk, and is unlinked from the segment list by walking the list.
* `releaseAll ()` returns every segment of the context to its arena at once,
  and `releaseAllToSystem ()` also tells the arena to release segments it has
  cached (see \ref behavior "Detailed Behavior").
* cookmem::CachedArena keeps freed segments in a size tree and hands out the
  smallest one that is large enough, whole.  It never splits or merges
  segments.

Memory is not returned within a segment: a segment that is partly in use
keeps all of its memory.

### User Size {#impl_user_size}

By default, a chunk records only its chunk size, and `getSize ()` returns the
usable size: the chunk size minus the header.  This can be larger than the
size that was requested.

When exact sizes are stored (`setStoringExactSize (true)`, or whenever
padding is on), `allocate ()` also records the requested size:

* If the request fills the chunk exactly, nothing more is stored.
* Otherwise `BIT_NOTEXACTSIZE` is set, and the last byte of the chunk holds
  the number of unused bytes at the end of the chunk.  `getSize ()` subtracts
  it from the usable size.

The unused space is always less than an alignment unit plus a minimum chunk
plus the padding, so it fits in one byte.  If `getSize ()` finds a byte that
is not positive, the chunk has been overwritten, and it throws
cookmem::Exception with cookmem::MEM_ERROR_PADDING.

dlmalloc does not store the requested size; `malloc_usable_size` returns the
usable size.

### Double Free Check

`deallocate ()` checks the used bit in the header of the chunk.  If it is not
set, the chunk is not in use, and `deallocate ()` throws cookmem::Exception
with cookmem::MEM_ERROR_DOUBLE_FREE.  Debug builds (`COOKMEM_DEBUG`) also
check that the header matches the footer kept by the next chunk.

`deallocate ()` clears the used bit before merging the chunk with its
neighbors.  If the chunk is merged into the free chunk before it, its header
is left inside the merged chunk, and the cleared bit still makes a second
free fail.

The check is cheap, so it cannot catch every double free:

* If the memory has been reused by a new allocation, the second free looks
  valid and frees the new allocation.
* A pointer that never came from the context is not detected.  Use
  `contains ()` to check whether a pointer is inside the segments of a
  context.

dlmalloc makes similar checks (`ok_inuse`, `ok_address`) and calls
`CORRUPTION_ERROR_ACTION` or `USAGE_ERROR_ACTION`, which abort by default.
CookMem throws exceptions instead.

### Out-of-Bounds Padding Check

A context created with a padding size (1 to cookmem::MAX_PADDING_SIZE, which
is 8) reserves extra bytes after every allocation:

    | header | user data (n bytes) | padding bytes | ... | size byte |
                                   ^ filled with the padding byte

* The request grows by the padding size plus one byte, so there is always
  room for the padding and for the exact-size byte at the end of the chunk.
* `allocate ()` fills the padding with the padding byte (0xcd by default; see
  `setPaddingByte ()`).
* `deallocate ()` checks that every padding byte is unchanged, and throws
  cookmem::Exception with cookmem::MEM_ERROR_PADDING if one was changed.
  This also happens when `reallocate ()` moves the memory, since it frees
  the old memory.

Padding turns on exact sizes, and they cannot be turned off while padding is
on.  The check only finds writes that land in the padding, and only when the
memory is freed.  dlmalloc has no equivalent.

### Debugging Aids

* `validate ()` walks every segment from its last chunk to its first through
  the footers, and checks that the sizes add up, that every free chunk is in
  the right bin or is the dv, that free chunks are never adjacent, and that
  the tree bin map matches the trees.  The checks use `COOKMEM_ASSERT`, so they
  only run in debug builds (`COOKMEM_DEBUG`).
* `print ()` prints every segment and chunk.
* cookmem::StatsLogger counts allocations by how they were satisfied (small
  bin, tree, dv, new segment or failure), along with segments and errors.

### dlmalloc Features That Are Not Implemented

* **The top chunk and `sbrk`.**  dlmalloc grows a contiguous "top" chunk with
  `MORECORE` (usually `sbrk`) and trims it when it gets too large
  (`malloc_trim`, `M_TRIM_THRESHOLD`).  CookMem only uses separate segments
  from the arena, and has no top chunk.
* **Direct mmap for large requests.**  dlmalloc maps requests above
  `M_MMAP_THRESHOLD` (256 KB by default) individually, with their own chunk
  format.  CookMem puts a large request in an ordinary segment of its own,
  and releases the segment when the allocation is freed (see above).
* **Merging contiguous segments**, which dlmalloc does when new memory is
  adjacent to an existing segment.
* **`realloc_in_place`**, which grows or shrinks only if the memory does not
  move.  `reallocate ()` does grow in place when it can.
* **Thread safety.**  dlmalloc can lock each mspace (`USE_LOCKS`).  CookMem
  contexts are not thread-safe (see \ref limitations "Limitations").
* **`FOOTERS` mode**, where each chunk records which mspace it belongs to,
  so that freeing into the wrong mspace is detected.
* **`independent_calloc`, `independent_comalloc` and `bulk_free`.**
* **`valloc` and `pvalloc`.**  `allocateAligned ()` covers `memalign` and
  `posix_memalign`.
* **`mallinfo`, `malloc_stats` and `malloc_inspect_all`.**  Use
  cookmem::StatsLogger, `getFootprint ()` and `getMaxFootprint ()` instead.
* **`mallopt` tuning.**  The settings that apply have CookMem equivalents:
  the minimum segment size of the arena, the trimming interval and the
  immediate release threshold.
