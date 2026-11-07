CookMem {#mainpage}
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

In the following table, the numbers are in seconds.

| Test           |  dlmalloc  | CookMem (RB-tree) | CookMem (AVL-tree) |
|----------------|:----------:|------------------:|-------------------:|
| perf_cookmem_1 | 0.191505   | 0.0987832         | 0.17094            |
| perf_cookmem_2 | 0.220995   | 0.13999           | 0.197844           |
| perf_cookmem_3 | 0.0214372  | 0.019327          | 0.0191321          |
| perf_cookmem_4 | 0.0318553  | 0.0248138         | 0.0250922          |

Use Cases {#usecases}
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
