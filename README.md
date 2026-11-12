CookMem
=======

[![Build Status](https://circleci.com/gh/coconut2015/cookmem.svg?style=shield)](https://app.circleci.com/pipelines/github/coconut2015/cookmem)[![codecov](https://codecov.io/gh/coconut2015/cookmem/branch/master/graph/badge.svg)](https://codecov.io/gh/coconut2015/cookmem)[![Apache License, Version 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

CookMem is a general-purpose [memory context](https://en.wikipedia.org/wiki/Region-based_memory_management)
and allocator written in C++11. It can be cleaned up quickly by freeing
segments rather than freeing allocations individually.

Unlike projects such as [nedmalloc](https://www.nedprod.com/programs/portable/nedmalloc/),
which is intended to replace ```malloc``` / ```free``` in multithreaded
applications, CookMem focuses on single-thread performance. It is intended
for use within a thread in an MPP process, avoiding synchronization costs for
most allocations and deallocations.
There are other use cases as well.

CookMem separates the logic for obtaining large memory segments, allowing
users to easily create a version of the memory context for their specific needs.

The algorithms and some code used here are based on [dlmalloc](http://gee.cs.oswego.edu/dl/html/malloc.html).
I took the time to understand dlmalloc and rewrite its logic in C++. Some key
calculations remain the same as in dlmalloc because there is no need to
reinvent the wheel.

Quick Start
-----------

CookMem is header-only.  Add the `include` directory to your include path and
include `cookmem.h`.

```cpp
#include <cookmem.h>
#include <vector>

int main ()
{
    // A memory context that obtains large memory segments from the system
    // (mmap or VirtualAlloc) and caches released segments for reuse.
    cookmem::CachedMemContext<> memCtx;

    // Allocate and deallocate like malloc and free.
    void* ptr = memCtx.allocate (100);
    memCtx.deallocate (ptr);

    // Memory with a larger alignment, for example for SIMD data.
    void* simd = memCtx.allocateAligned (256, 64);
    memCtx.deallocate (simd);

    // Standard containers can allocate from the same context.
    cookmem::Allocator<int> allocator (memCtx);
    std::vector<int, cookmem::Allocator<int>> numbers (allocator);
    numbers.push_back (42);

    return 0;
}   // Destroying memCtx releases all of its memory at once.
```

See the [tutorials](https://coconut2015.github.io/cookmem/tutorials.html) for more.

Requirements
------------

* A C++11 compiler.  CookMem is tested with GCC 13 on Linux and with
  Visual Studio 2022 on Windows, on both 32-bit and 64-bit platforms.
* [CMake](https://cmake.org/) 3.14 or later, to use the `cookmem::cookmem`
  target or to build the tests, examples and documentation.  CMake is not
  needed to use the headers directly.  See
  [Installation](https://coconut2015.github.io/cookmem/install.html) and
  [Build Instructions](https://coconut2015.github.io/cookmem/build.html).

Performance
-----------

CookMem is faster than dlmalloc in most of the tests below, up to about 13
times faster for large blocks that come straight from the arena
(perf_cookmem_10), and about the same in the others.  dlmalloc is faster for
small blocks of the same size that are allocated and freed repeatedly
(perf_cookmem_5), and for a sliding window of blocks of mixed sizes
(perf_cookmem_11).  Neither dlmalloc nor CookMem uses locks.  CookMem was
tested with both of its large chunk bins: the red-black tree (RB-tree), which
is the default, and the AVL tree.

An [interactive chart](https://coconut2015.github.io/cookmem/performance.html) of these results is
available with the documentation.

The tests were run on an AMD Ryzen 7 5800H with Ubuntu 24.04 (WSL2), built by
GCC 13.3 with `-O3`.  Times vary between runs by about 10%, so small
differences are not significant.  Each test was run 5 times. For each column, the best and the worst
times were dropped and the remaining times were averaged. The numbers are
in seconds.

| Test | What it covers | dlmalloc | CookMem (RB-tree) | CookMem (AVL-tree) |
|------|----------------|---------:|------------------:|-------------------:|
| perf_cookmem_1 | Copy a 200-character string into a vector 1M times (small bins, plus large vector growth) | 0.137765 | 0.112946 | 0.131251 |
| perf_cookmem_2 | Same as perf_cookmem_1, then clear and fill again (reuse of freed chunks) | 0.253331 | 0.168729 | 0.178487 |
| perf_cookmem_3 | 10k strings of growing length 1 to 10000 (mostly tree bins) | 0.024164 | 0.022851 | 0.022613 |
| perf_cookmem_4 | Same as perf_cookmem_3, then clear and build again (reuse of freed tree chunks) | 0.036800 | 0.026149 | 0.026622 |
| perf_cookmem_5 | Small same-size churn (`std::list<int>`, small bins) | 0.069674 | 0.079835 | 0.084755 |
| perf_cookmem_6 | Mixed sizes (`std::map<int, std::string>`, values 0-999 chars, erase and re-insert) | 0.149489 | 0.164420 | 0.161334 |
| perf_cookmem_7 | 50k free chunks of the same 1KB size, then merging with free neighbors | 0.045692 | 0.022891 | 0.022319 |
| perf_cookmem_8 | Fragmentation and best-fit search (sizes 256 B-8 KB, refill the holes) | 0.081027 | 0.071846 | 0.077851 |
| perf_cookmem_9 | Interleaved `realloc` growth up to 32KB | 0.598881 | 0.567728 | 0.585602 |
| perf_cookmem_10 | Large blocks (128KB-4MB) going straight to the arena | 0.029663 | 0.002250 | 0.002693 |
| perf_cookmem_11 | Sliding window (FIFO) of 1024 live blocks, 16-2047 bytes | 0.087624 | 0.109254 | 0.122220 |
| perf_cookmem_12 | 10k vectors grown in interleaved order | 0.023026 | 0.022922 | 0.022312 |

Use Cases
=========

Avoiding Synchronization Cost
-----------------------------

In multithreaded applications, `malloc` / `free` or `new` / `delete` must
synchronize on each call. This synchronization cost can be significant.
nedmalloc exists to reduce the synchronization costs of naive implementations
and thereby increase performance.

For performance-critical applications, eliminating much of this synchronization
cost can be even more beneficial. With CookMem, obtaining large segments using
calls such as `mmap` may require synchronization, but allocations within those
segments do not.  See
[Thread-local and Scoped Context STL Allocators](https://coconut2015.github.io/cookmem/tutorials.html#tlsSTLAlloc).

Avoiding Individual Frees
-------------------------

There are two sub-cases.

1. Doing individual frees can reduce performance.
2. A complex pointer graph containing trees or cycles can make it very difficult
   to free pointers in the correct order. Yet, the memory has to be released to
   avoid memory leaks.

Both cases can be addressed easily by using any of the MemContext classes.
Destroying a memory context releases all of its memory at once.  See
[Detailed Behavior](https://coconut2015.github.io/cookmem/behavior.html) for the details.

Restricting Memory Usage
------------------------

For long-running programs that take user inputs, it is possible that even when all input
values are validated, edge cases in calculations can still cause the program to
accidentally request a very large amount of memory and crash. CookMem provides
several ways to restrict memory usage. One can set a memory footprint limit or
add code to the segment request logic to restrict large memory requests.
See [Restricting Memory Usage](https://coconut2015.github.io/cookmem/tutorials.html#footprint).

Diagnosing and Debugging
------------------------

CookMem has built-in diagnostic tools that can help gather memory statistics
and detect difficult-to-catch out-of-bounds writes without significantly
slowing down the program. It also detects attempts to free an allocation more
than once. CookMem is not a [Valgrind](https://valgrind.org/) replacement, but
in many cases it can help catch difficult bugs.  See
[Memory Statistics](https://coconut2015.github.io/cookmem/tutorials.html#statistics) and
[Memory Corruption Detection](https://coconut2015.github.io/cookmem/tutorials.html#boundcheck).

Links
-----

* Documentation: https://coconut2015.github.io/cookmem/
  * [Tutorials](https://coconut2015.github.io/cookmem/tutorials.html)
  * [Detailed Behavior](https://coconut2015.github.io/cookmem/behavior.html)
  * [Limitations](https://coconut2015.github.io/cookmem/limitations.html)
  * [Implementation Details](https://coconut2015.github.io/cookmem/implementation.html)
  * [Installation](https://coconut2015.github.io/cookmem/install.html)
  * [Build Instructions](https://coconut2015.github.io/cookmem/build.html)
  * [References](https://coconut2015.github.io/cookmem/references.html)
  * [Performance Chart](https://coconut2015.github.io/cookmem/performance.html)
* Source: https://github.com/coconut2015/cookmem
