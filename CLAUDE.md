# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Project

CookMem is a header-only C++11 memory context and allocator based on
dlmalloc.  All library code is in `include/` (`cookmem.h` includes
everything under `include/cookmem/`).  See `doxygen/implementation.md` for
how the allocator works and how it differs from dlmalloc.

Layers, combined through templates rather than virtual calls:

- **Arenas** (`arena.h`, `mmaparena.h`, `mallocarena.h`) provide segments.
  They implement the `MemArena` interface by duck typing and do not inherit
  from it.  `MemArena` itself stays a pure virtual interface for users who
  want runtime polymorphism.
- **`MemPool`** (`pool.h`) implements the dlmalloc algorithms: chunks, small
  bins, tree bins (`rbtree.h` / `avltree.h`), the designated victim and
  segments.
- **Contexts** (`context.h`) wrap a pool with an arena and a logger.
- **Loggers** (`logger.h`) and the STL **`Allocator`** with context providers
  (`allocator.h`).

## Build and test

Linux / WSL (`make.sh` builds in `debug/`, `release/`, `cov/`, ...):

```bash
./make.sh            # debug build + tests
./make.sh release    # release build + tests, including performance tests
./make.sh cov        # coverage report in cov/test_coverage (CI uses this)
./make.sh docs       # Doxygen into docs/
```

32-bit Linux: `cmake -S . -B <dir> -DCMAKE_CXX_FLAGS=-m32` (needs
g++-multilib).

Windows: `make.cmd` builds in `win-<config>` folders.  From WSL, use a batch
file that calls `VsDevCmd.bat` and run it with `cmd.exe /c`; a plain
`cmd.exe` has no cmake on its PATH.  Test both `-A x64` and `-A Win32`.

Before calling a change done, run the tests on Linux x64 and i386 and on
MSVC x64 and Win32, in Debug and Release.  Debug builds define
`COOKMEM_DEBUG`, which turns on `COOKMEM_ASSERT` and the checks in
`validate ()`.  Keep coverage at its current level (about 99.6%) and Doxygen
free of warnings.

## Git and GitHub from WSL

- `gh` is installed on Windows only: use `gh.exe`.
- WSL `git` has no GitHub credentials.  Commit with WSL `git`, but push with
  `git.exe`.
- Write PR and issue bodies to a file under `D:\workspace\tmp`
  (`/mnt/d/workspace/tmp`) and pass `--body-file`, since quoting through WSL
  breaks.
- Work on a branch and open a PR to `master`; do not push to `master`.

## Conventions

- C++11 only, and the library stays header-only.
- Style: four-space indents, a space before the parentheses of calls and
  declarations (`allocate (size)`), the return type on its own line for
  functions defined in classes, and Doxygen `/** */` comments on every
  public member.
- The library throws `cookmem::Exception` for misuse it detects (double
  free, padding errors) and returns `nullptr` when an allocation fails.
- Sizes and alignment must work on 32-bit platforms: derive masks from
  `ALLOCATION_ALIGNMENT` (two `size_t`) instead of hardcoding 16, and avoid
  64-bit literals in tests.
- Every bug fix gets a test that fails without the fix.

## Tests

- Tests are plain executables in `tests/`, using the macros in
  `tests/unittest.h` (`ASSERT_EQ`, `ASSERT_NE`).  Each `testN ()` returns 0
  and is called from `main` inside the `try` block.
- Register a new test in `CMakeLists.txt` with `add_executable` and
  `add_test`.
- Examples in `examples/` are built and run as tests too.

## Releases

1. Bump `COOKMEM_VERSION_MINOR` / `PATCH` in `include/cookmem/version.h`.
   CMake reads the version from this header, and `test_version` checks it.
2. Update the version in `doxygen/install.md` (`GIT_TAG` and
   `find_package`).
3. After the PR is merged, tag `vX.Y.Z` on `master` and create the GitHub
   release.
4. The vcpkg and Conan recipes in `packaging/` need the hashes of the
   release archive; see `packaging/README.md`.

## Documentation

User documentation is Markdown in `doxygen/`, listed in the `INPUT` of
`doxygen/Doxyfile.in`, and published to GitHub Pages by a workflow.  Update
`behavior.md`, `limitations.md` or `implementation.md` when behavior
changes.
