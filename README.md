CookMem {#mainpage}
=======

[![Build Status](https://travis-ci.org/coconut2015/cookmem.svg?branch=master)](https://travis-ci.org/coconut2015/cookmem)[![codecov](https://codecov.io/gh/coconut2015/cookmem/branch/master/graph/badge.svg)](https://codecov.io/gh/coconut2015/cookmem)[![Apache License, Version 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

CookMem is a [memory context](https://en.wikipedia.org/wiki/Region-based_memory_management)
/ allocator written in C++11 that can be easily and quickly cleaned up by
freeing the segments, rather than doing individual frees.  It is extremely
useful to deal with third party libraries that can potentially have memory
leaks, or to simplify the memory management.

The secondary goal of this project is to provide a good enough performance
that should be comparable to [dlmalloc](http://g.oswego.edu/dl/html/malloc.html)
and its derivatives.  In fact, the algorithms and some code used here are
mostly based on dlmalloc.  I basically took the pain to understand dlmalloc
and rewrote the logics in C++.  Some key calculations though are kept the
same as dlmalloc since there is no point to reinvent wheels.

The project documentation is at http://coconut2015.github.io/cookmem/
