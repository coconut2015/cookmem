CookMem
=======

[![Build Status](https://circleci.com/gh/coconut2015/cookmem.svg?style=shield)](https://app.circleci.com/pipelines/github/coconut2015/cookmem)[![codecov](https://codecov.io/gh/coconut2015/cookmem/branch/master/graph/badge.svg)](https://codecov.io/gh/coconut2015/cookmem)[![Apache License, Version 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

CookMem is a general-purpose [memory context](https://en.wikipedia.org/wiki/Region-based_memory_management)
and allocator written in C++11. It can be cleaned up quickly by freeing
segments rather than freeing allocations individually.

CookMem is not intended as a drop-in replacement for `malloc` / `free`, unlike
[nedmalloc](https://www.nedprod.com/programs/portable/nedmalloc/),
[jemalloc](https://jemalloc.net/) and
[mimalloc](https://github.com/microsoft/mimalloc), which replace the
allocator of a whole multithreaded program.  CookMem targets single-threaded
use: code allocates from a memory context explicitly, and each context is
used by one thread at a time, for example one context for each thread of an
MPP process.  This avoids synchronization for most allocations and
deallocations.  See "Who CookMem Is For" below.

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

* A C++11 compiler.  CookMem is tested with GCC and Clang on Linux (x64,
  i386 and arm64), and with Visual Studio 2022 (MSVC and clang-cl) on
  Windows (x64 and Win32).  With C++17, `cookmem::MemResource` also provides
  a `std::pmr::memory_resource` for the `std::pmr` containers.
* [CMake](https://cmake.org/) 3.14 or later, to use the `cookmem::cookmem`
  target or to build the tests, examples and documentation.  CMake is not
  needed to use the headers directly.  See
  [Installation](https://coconut2015.github.io/cookmem/install.html) and
  [Build Instructions](https://coconut2015.github.io/cookmem/build.html).

Performance
-----------

CookMem is faster than dlmalloc in most of the tests below: about 13 times
faster for large blocks that come straight from the arena (perf_cookmem_10),
about 5 times faster for interleaved `realloc` growth, where blocks grow in
place (perf_cookmem_9), and about the same in the others.  dlmalloc is faster
for a sliding window of blocks of mixed sizes (perf_cookmem_11).  Neither dlmalloc nor CookMem uses locks.  CookMem was
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
| perf_cookmem_1 | Copy a 200-character string into a vector 1M times (small bins, plus large vector growth) | 0.179852 | 0.147207 | 0.138198 |
| perf_cookmem_2 | Same as perf_cookmem_1, then clear and fill again (reuse of freed chunks) | 0.328629 | 0.190954 | 0.177236 |
| perf_cookmem_3 | 10k strings of growing length 1 to 10000 (mostly tree bins) | 0.028434 | 0.026320 | 0.026967 |
| perf_cookmem_4 | Same as perf_cookmem_3, then clear and build again (reuse of freed tree chunks) | 0.045815 | 0.032542 | 0.031780 |
| perf_cookmem_5 | Small same-size churn (`std::list<int>`, small bins) | 0.108232 | 0.086955 | 0.095069 |
| perf_cookmem_6 | Mixed sizes (`std::map<int, std::string>`, values 0-999 chars, erase and re-insert) | 0.162361 | 0.174398 | 0.159243 |
| perf_cookmem_7 | 50k free chunks of the same 1KB size, then merging with free neighbors | 0.058387 | 0.028435 | 0.030270 |
| perf_cookmem_8 | Fragmentation and best-fit search (sizes 256 B-8 KB, refill the holes) | 0.105328 | 0.093511 | 0.097044 |
| perf_cookmem_9 | Interleaved `realloc` growth up to 32KB | 0.503400 | 0.088520 | 0.092882 |
| perf_cookmem_10 | Large blocks (128KB-4MB) going straight to the arena | 0.039517 | 0.003039 | 0.002812 |
| perf_cookmem_11 | Sliding window (FIFO) of 1024 live blocks, 16-2047 bytes | 0.094661 | 0.117984 | 0.122740 |
| perf_cookmem_12 | 10k vectors grown in interleaved order | 0.028886 | 0.026630 | 0.028787 |

Who CookMem Is For
==================

CookMem helps most in four kinds of programs.

Complex Pointer Graphs
----------------------

Some programs build large structures of objects that point to each other:
trees, directed acyclic graphs and cycles.  Freeing such a structure one
object at a time, in an order that never leaves a dangling pointer, is
error-prone, and every object that is missed leaks.  With CookMem, the
structure is allocated from a memory context and released all at once, with
`releaseAll ()` or by destroying the context.

* Compilers and interpreters: abstract syntax trees, symbol tables, type
  graphs and intermediate representations.
* Lexers and parsers: JSON, XML and HTML document trees, regular expression
  automata, and template engines.
* Database engines: query plans, optimizer search spaces, and the memory of
  a single query or transaction.
* Servers that allocate everything for one request and drop it when the
  request ends.
* Symbolic math and theorem provers (expression and term graphs), CAD
  meshes, document layout trees, and game scenes that are loaded and
  unloaded as a unit.

Releasing everything at once frees the memory without calling destructors,
so C++ RAII does not happen: it suits objects that own nothing but memory
from the same context, and objects that hold files, locks or memory from the
global heap must be destroyed first.

Unlike simple arena (bump) allocators, a memory context can also free
individual objects and reuse their memory, so long-lived structures that
change over time do not keep growing.  See
[Detailed Behavior](https://coconut2015.github.io/cookmem/behavior.html).

Single-Threaded Performance
---------------------------

Unless an algorithm is parallel, it runs in one thread.  So does most of
the work in a multithreaded program: a worker handles one request, one
partition or one core at a time.  A memory context that is used by one
thread needs no synchronization.  It also keeps related allocations close
together in memory, frees them in bulk, and grows an allocation in place
when the memory after it is free.

* Thread-per-core servers and MPP database executors, with a context for
  each worker thread.
* Batch jobs, simulations and game loops that release a context after each
  batch or frame.
* Latency-sensitive code, such as audio processing or order books, where a
  cookmem::FixedArena over a preallocated buffer avoids system calls.

Modern system allocators also avoid locks with per-thread caches, so measure
with your own workload; the performance results above compare CookMem
with dlmalloc.  See
[Thread-local and Scoped Context STL Allocators](https://coconut2015.github.io/cookmem/tutorials.html#tlsSTLAlloc).

Debugging Where Valgrind Is Impractical
---------------------------------------

[Valgrind](https://valgrind.org/) typically makes a program 20 to 30 times
slower.  That changes timing, breaks timeouts, and makes long-running
services or production-sized data impractical to check.  Valgrind does not
run on Windows, and AddressSanitizer needs a special build and uses much
more memory.  CookMem's checks are cheap enough to keep on in a normal
build, for each context separately:

* Padding after each allocation detects writes past its end when the
  allocation is freed.
* Freeing an allocation twice throws cookmem::Exception.
* `forEachAllocation ()` lists the allocations that were never freed, for
  example at the end of each request.
* cookmem::StatsLogger gathers allocation statistics.

CookMem is not a Valgrind replacement: it catches fewer kinds of errors, but
it runs where Valgrind cannot.  See
[Memory Corruption Detection](https://coconut2015.github.io/cookmem/tutorials.html#boundcheck),
[Finding Memory Leaks](https://coconut2015.github.io/cookmem/tutorials.html#leaks) and
[Memory Statistics](https://coconut2015.github.io/cookmem/tutorials.html#statistics).

Limiting Memory Use
-------------------

A footprint limit caps the memory a context can obtain.  When the limit is
reached, allocations fail, with `nullptr` or with `std::bad_alloc` through
the STL adapters, instead of the whole process running out of memory and
crashing.

* Third-party libraries that accept custom allocation functions, as many C
  libraries do: Lua (`lua_newstate`), SQLite (`SQLITE_CONFIG_MALLOC`), zlib
  (`zalloc` and `zfree`), FreeType, expat, PCRE2 and libpng.  Each can get
  its own context and limit.
* Parsers and decoders of untrusted input, such as images, documents and
  compressed data, where a malicious file could request huge amounts of
  memory.
* Multi-tenant services with a memory quota for each tenant or request, and
  database operators with a memory budget.

The limit applies only to memory allocated through the context.  Code that
calls `malloc` or `new` directly bypasses it, so this is not process
isolation.  See
[Restricting Memory Usage](https://coconut2015.github.io/cookmem/tutorials.html#footprint).

Other Uses
----------

* Embedded systems and game consoles: cookmem::FixedArena over a static
  buffer gives a fixed memory budget without system calls.
* Memory accounting: with a context for each subsystem, `getFootprint ()`
  and cookmem::StatsLogger show which part of a program uses the memory.
* Special memory: a custom arena can supply huge pages, memory local to a
  NUMA node, or memory locked in RAM.
* Fast shutdown: short-lived tools can release everything at exit instead
  of freeing objects one by one.

Links
=====

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
