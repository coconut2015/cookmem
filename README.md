CookMem
=======

[![Build Status](https://travis-ci.org/coconut2015/cookmem.svg?branch=master)](https://travis-ci.org/coconut2015/cookmem)[![Apache License, Version 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)


CookMem is a a memory context library written in C++11 that can be easily
cleaned up simply by freeing the segments, rather than doing individual
frees.  It is useful to deal with third party libraries that can potentially
have memory leaks, or to simplify the memory management.

Currently, the memory context does not allow locking, and thus is only
suitable for use within a thread.  In the future, a locking facility
could be provided.

The algorithm and some code used here is based on dlmalloc
( http://g.oswego.edu/dl/html/malloc.html ).

The goal of this utility is not necessarily providing the fastest memory
allocator, but one with good enough performance.

Installation Instruction
------------------------

CookMem is simply a collection of C++ header files.  Thus, there are no
libraries to install.  It is only necessarily to put the files in
a suitable include directory.

Test Build Instruction
----------------------

Run the following command to build.

```bash
./make.sh [type]
```

``type`` can be one of the following:

* ``doc`` - for generating Doxygen documents.
* ``debug`` - for building tests for debugging purposes.
* ``profiler`` - for building tests with profilers.
* ``release`` - for building tests with optimization enabled.  It is for
  building internal performance tests.

License
-------

Copyright 2018 by Heng Yuan

This software is provided under
[Apache Public License](https://www.apache.org/licenses/LICENSE-2.0.html).
