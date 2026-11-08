Build Instructions {#build}
------------------

CookMem is header-only, so building is only needed to run the tests,
examples and performance tests, or to generate the documentation.  To use
CookMem in your own project, see \ref install "Installation".

### Prerequisites

* [CMake](https://cmake.org/) 3.12 or later.
* A C++11 compiler, such as GCC, Clang or Visual Studio.
* For code coverage (Linux `debug` builds): `gcov`, `lcov` and `genhtml`.
* For documentation: [Doxygen](https://www.doxygen.nl/) and
  [Graphviz](https://graphviz.org/) (for the class diagrams).

On Ubuntu, for example:

```bash
sudo apt-get install g++ cmake lcov doxygen graphviz
```

### Linux

```bash
./make.sh [type] [-t test]
```

`type` can be one of the following.  The default is `debug`.  Each type
builds in a directory of the same name.

* `debug` - builds with debug information and runs the tests.  The internal
  assertions are enabled (`COOKMEM_DEBUG`), and a code coverage report is
  generated at `debug/test_coverage/index.html`.
* `release` - builds with optimization and runs the tests, including the
  performance tests.
* `profiler` - like `release`, but built with `-pg` for profiling with
  `gprof`.
* `docs` - generates the Doxygen documentation into the `docs` directory.
* `help` - prints the usage.

`-t test` builds and runs only one test, for example
`./make.sh debug -t test_rbtree`.  No coverage report is generated in this
case.

### Windows

```bat
make.cmd [type]
```

`type` can be `debug` (the default), `release` or `profiler`.  It builds with
CMake's default generator, normally Visual Studio, and runs the tests.

Code coverage, documentation and the performance tests are only available on
Linux.

### Using CMake Directly

The scripts are wrappers around CMake.  For example:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
ctest
```

Pass `-DCOVERAGE=1` for code coverage (with a `Debug` build) or
`-DBUILD_DOC=1` for documentation.

### Performance Tests

The performance tests compare CookMem with
[dlmalloc](http://gee.cs.oswego.edu/dl/html/malloc.html), using both the
red-black tree and the AVL tree.  They are built by the `release` type and
only run on Linux.

```bash
./make.sh release
./perftest.sh [build directory] [report file]
```

`perftest.sh` runs each test 5 times (set `RUNS` to change this), drops the
best and the worst time of each allocator, and averages the rest.  It prints
a markdown table that can be pasted into `README.md`.  It also writes the
results to `performances/perfdata.js`, which the
[performance chart](performance.html) displays.
