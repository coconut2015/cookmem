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

Performance
-----------

The performance of CookMem is comparable to, if not faster than, dlmalloc.
Neither dlmalloc nor CookMem uses locks. For CookMem, I also tested red-black
tree (RB-tree) which is the default, and AVL-tree backend.

An [interactive chart](http://coconut2015.github.io/cookmem/performance.html)
of these results is available with the documentation.

Each test was run 5 times. For each column, the best and the worst
times were dropped and the remaining times were averaged. The numbers are
in seconds.

| Test | What it covers | dlmalloc | CookMem (RB-tree) | CookMem (AVL-tree) |
|------|----------------|---------:|------------------:|-------------------:|
| perf_cookmem_1 | Copy a 200-character string into a vector 1M times (small bins, plus large vector growth) | 0.126525 | 0.101618 | 0.099559 |
| perf_cookmem_2 | Same as perf_cookmem_1, then clear and fill again (reuse of freed chunks) | 0.233960 | 0.140242 | 0.141031 |
| perf_cookmem_3 | 10k strings of growing length 1 to 10000 (mostly tree bins) | 0.022157 | 0.020473 | 0.020203 |
| perf_cookmem_4 | Same as perf_cookmem_3, then clear and build again (reuse of freed tree chunks) | 0.033466 | 0.025645 | 0.025124 |
| perf_cookmem_5 | Small same-size churn (`std::list<int>`, small bins) | 0.068185 | 0.074804 | 0.079774 |
| perf_cookmem_6 | Mixed sizes (`std::map<int, std::string>`, values 0-999 chars, erase and re-insert) | 0.160800 | 0.149217 | 0.153573 |
| perf_cookmem_7 | 50k free chunks of the same 1KB size, then merging with free neighbors | 0.045228 | 0.022644 | 0.023003 |
| perf_cookmem_8 | Fragmentation and best-fit search (sizes 256 B-8 KB, refill the holes) | 0.079755 | 0.070487 | 0.069969 |
| perf_cookmem_9 | Interleaved `realloc` growth up to 32KB | 0.579950 | 0.590824 | 0.580901 |
| perf_cookmem_10 | Large blocks (128KB-4MB) going straight to the arena | 0.029976 | 0.002192 | 0.002443 |
| perf_cookmem_11 | Sliding window (FIFO) of 1024 live blocks, 16-2047 bytes | 0.091809 | 0.115924 | 0.130731 |
| perf_cookmem_12 | 10k vectors grown in interleaved order | 0.023595 | 0.022747 | 0.023023 |

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
segments do not.

Avoiding Individual Frees
-------------------------

There are two sub-cases.

1. Doing individual frees can reduce performance.
2. A complex pointer graph containing trees or cycles can make it very difficult
   to free pointers in the correct order. Yet, the memory has to be released to
   avoid memory leaks.

Both cases can be addressed easily by using any of the MemContext classes.

Restricting Memory Usage
------------------------

For long-running programs that take user inputs, it is possible that even when all input
values are validated, edge cases in calculations can still cause the program to
accidentally request a very large amount of memory and crash. CookMem provides
several ways to restrict memory usage. One can set a memory footprint limit or
add code to the segment request logic to restrict large memory requests.

Diagnosing and Debugging
------------------------

CookMem has built-in diagnostic tools that can help gather memory statistics
and detect difficult-to-catch out-of-bounds writes without significantly
slowing down the program. It also detects attempts to free an allocation more
than once. CookMem is not a [Valgrind](https://valgrind.org/) replacement, but
in many cases it can help catch difficult bugs.

Links
-----

* Documentation: http://coconut2015.github.io/cookmem/
* Source: https://github.com/coconut2015/cookmem
