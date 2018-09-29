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
