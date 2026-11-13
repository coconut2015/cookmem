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

Other Linux builds, each in a separate build directory:

- Clang: `-DCMAKE_CXX_COMPILER=clang++`.
- 32-bit: `-DCMAKE_CXX_FLAGS=-m32`, with GCC or Clang.  It needs the 32-bit
  runtime and the `/usr/include/asm` link from `g++-multilib`.

arm64 is tested only by CI, on native arm64 runners.

Before calling a change done, run the tests in Debug and Release with GCC
and Clang on Linux x64 and i386, and with MSVC on x64 and Win32.  CI also
runs arm64 builds and clang-cl on Windows.  Debug builds define
`COOKMEM_DEBUG`, which turns on `COOKMEM_ASSERT` and the checks in
`validate ()`.  Keep coverage at its current level (about 99.6%) and Doxygen
free of warnings.

Performance: for changes to allocation or deallocation paths, compare with
`master` on release builds of both with
`bash perfcompare.sh <master build dir> <branch build dir>`.  It runs
`perftest.sh` on the two builds alternately and prints the change per test.
Times vary by about 10% between runs; the dlmalloc column is the same code in
both builds and shows the noise.  For arm64, run the Performance workflow
(`.github/workflows/perf.yml`): add the `perf` label to the PR, or once the
workflow is on `master`, run
`gh.exe workflow run perf.yml --ref <branch> -f arch=arm64`.  When the results
change, regenerate the README table and `performances/perfdata.js` with
`./perftest.sh <build dir>`, and update the summary above the table.

## Git and GitHub from WSL

- `gh` is installed on Windows only: use `gh.exe`.
- WSL `git` has no GitHub credentials.  Commit with WSL `git`, but push with
  `git.exe`.
- Push with `git.exe -c credential.interactive=never push ...`.  Without it,
  a missing Git Credential Manager login makes the push wait forever for a
  sign-in window.  If the login is missing, the push can use gh's login
  instead:
  `git.exe -c credential.helper= -c 'credential.helper=!"C:/Program Files/GitHub CLI/gh.exe" auth git-credential' push ...`.
  gh's token has no `workflow` scope, so GitHub rejects this for a branch
  that changes `.github/workflows/`; ask the user to sign in or run
  `gh auth refresh -h github.com -s workflow`.
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
- Code is also compiled by MSVC, where `windows.h` defines macros such as
  `small`, `min` and `max`.  Do not use them as names.

## Tests

- Tests are plain executables in `tests/`, using the macros in
  `tests/unittest.h` (`ASSERT_EQ`, `ASSERT_NE`).  Each `testN ()` returns 0
  and is called from `main` inside the `try` block.
- Register a new test in `CMakeLists.txt` with `add_executable` and
  `add_test`.
- Examples in `examples/` are built and run as tests too.
- Tests that depend on where chunks are placed should create a new context
  for that case, since earlier frees change the layout.  A `MmapArena` with
  a minimum segment size of 0 gives segments of exactly the requested size,
  which leaves no free remainder (see `ExactContext` in
  `tests/test_cookmem_04.cpp`).

## Releases

1. Bump `COOKMEM_VERSION_MINOR` / `PATCH` in `include/cookmem/version.h`.
   CMake reads the version from this header, and `test_version` checks it.
2. Update the version in `doxygen/install.md` (`GIT_TAG` and
   `find_package`).
3. After the PR is merged, tag `vX.Y.Z` on `master` and create the GitHub
   release.  Then check that a new CMake project can use the tag with
   `FetchContent`.
4. The vcpkg and Conan recipes in `packaging/` need the hashes of the
   release archive; see `packaging/README.md`.

## Documentation

User documentation is Markdown in `doxygen/`, listed in the `INPUT` of
`doxygen/Doxyfile.in`, and published to GitHub Pages by a workflow.  Update
`behavior.md`, `limitations.md` or `implementation.md` when behavior
changes.
