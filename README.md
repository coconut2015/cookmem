CookMem {#mainpage}
=======

[![Build Status](https://circleci.com/gh/coconut2015/cookmem.svg?style=shield)](https://app.circleci.com/pipelines/github/coconut2015/cookmem)[![codecov](https://codecov.io/gh/coconut2015/cookmem/branch/master/graph/badge.svg)](https://codecov.io/gh/coconut2015/cookmem)[![Apache License, Version 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

CookMem is a [memory context](https://en.wikipedia.org/wiki/Region-based_memory_management)
/ allocator written in C++11 that can be easily and quickly cleaned up by
freeing the segments, rather than doing individual frees.

Unlike projects such as [nedmalloc](https://www.nedprod.com/programs/portable/nedmalloc/)
which is intended to replace ```malloc``` / ```free``` in multi-threaded
applications, this project focuses on the single-thread performance.  It is
intended to be used within a thread in an MPP process, such that it would
not have to pay for the cost of synchronization for tiny
allocations / deallocations.

Additionally, cookmem separates out the logic for obtaining large segments
of memory, to allow users easily creating their flavor of the memory context
for their specific needs.

The algorithms and some code used here are based on [dlmalloc](http://gee.cs.oswego.edu/dl/html/malloc.html).
I basically took the pain to understand dlmalloc and rewrote the logics
in C++.  Some key calculations though are kept the same as dlmalloc since
there are no points to reinvent wheels.

Since I have never learnt [Red-black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree),
which dlmalloc uses.  [AVL tree](https://en.wikipedia.org/wiki/AVL_tree)
is used instead.  There can be some minor performance trade offs.  Maybe one
day I will learn the RB-tree and check the difference ;)

* Documention: http://coconut2015.github.io/cookmem/
* Source: https://github.com/coconut2015/cookmem
