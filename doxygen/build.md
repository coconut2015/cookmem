Test Build Instruction {#build}
----------------------

Run the following command to build.

```bash
./make.sh [type]
```

``type`` can be one of the following:

* ``doc`` - for generating Doxygen documents.
* ``debug`` - for building tests for debugging purposes.  Code coverage report
  can be seen at debug/test_coverage/index.html
* ``profiler`` - for building tests with profilers.
* ``release`` - for building tests with optimization enabled.  It is for
  building internal performance tests.
