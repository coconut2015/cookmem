CookMem {#mainpage}
=======

[![Build Status](https://circleci.com/gh/coconut2015/cookmem.svg?style=shield)](https://app.circleci.com/pipelines/github/coconut2015/cookmem)[![codecov](https://codecov.io/gh/coconut2015/cookmem/branch/master/graph/badge.svg)](https://codecov.io/gh/coconut2015/cookmem)[![Apache License, Version 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

CookMem is a general purpose [memory context](https://en.wikipedia.org/wiki/Region-based_memory_management)
/ allocator written in C++11 that can be easily and quickly cleaned up by
freeing the segments, rather than doing individual frees.

Unlike projects such as [nedmalloc](https://www.nedprod.com/programs/portable/nedmalloc/)
which is intended to replace ```malloc``` / ```free``` in multithreaded
applications, this project focuses on the single-thread performance.  It is
intended to be used within a thread in an MPP process, such that it would
not have to pay for the cost of synchronization for most allocations / deallocations.
There are other use cases as well.

CookMem separates out the logic for obtaining large segments
of memory, to allow users easily creating their flavor of the memory context
for their specific needs.

The algorithms and some code used here are based on [dlmalloc](http://gee.cs.oswego.edu/dl/html/malloc.html).
I basically took the pain to understand dlmalloc and rewrote the logics
in C++.  Some key calculations though are kept the same as dlmalloc since
there are no points to reinvent wheels.

Performance
-----------

The performance of CookMem is comparable if not faster than dlmalloc.  There
are no locking for dlmalloc or CookMem.  For CookMem, I also tested red-black
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

In multi-threaded applications, `malloc` / `free` or `new` / `delete` have to synchronize
for each call.  This synchronization cost can be significant.  This is why
nedmalloc exists, to reduce the synchronization costs due to naive implementations
and thereby increasing the performance.

For performance critical applications though, being able to eleminate much of
synchronization cost is even more beneficial.  With CookMem, only getting
large segments using calls such as `mmap` would require synchronization, but
allocations within the segments do not require synchronizations at all.

Avoiding Individual Frees
-------------------------

There are two sub-cases.

1. Doing individual frees can cost performance.
2. If the program has a complex pointer graph, where trees or cycles exist.  It becomes
   very difficult to free pointers in the correct order.  Yet, the memory has to be
   released to avoid memory leak.

Both cases can be easily addressed using CookMem using any one of the MemContext classes.

Restrict Memory Usage
---------------------

For long running programs that take user inputs, it is possible that even when all input
values are validated, there can be edge cases in the calculations that can result
in situations where one could accidentally request very large amount of memory
that can crash the program.  CookMem has a number of ways of restricting memory usage.
One could either set the memory footprint limit, or add the code in segment request
to restrict large memory requests.

Diagnosing and Debugging
------------------------

CookMem has built-in diagnostic tools that can help gather memory statistics, and
detect difficult to catch out-of-bound writes without significantly slowly down
the program.  It is certainly not a [Valgrind](https://valgrind.org/) replacement,
but in many cases, it is good enough to catch some hard to deal with bugs.

Links
-----

* Documentation: http://coconut2015.github.io/cookmem/
* Source: https://github.com/coconut2015/cookmem
