Installation {#install}
------------

CookMem is a collection of C++ header files, so there are no libraries to
build or link.  Include `cookmem.h` in your code.

### Using CMake

CookMem provides the CMake target `cookmem::cookmem`.  Linking it adds the
CookMem include directory and requires C++11:

```cmake
target_link_libraries (myapp PRIVATE cookmem::cookmem)
```

Use one of the following to make the target available.  They require CMake
3.14 or later.

**FetchContent** downloads CookMem when your project is configured:

```cmake
include (FetchContent)
FetchContent_Declare (cookmem
    GIT_REPOSITORY https://github.com/coconut2015/cookmem.git
    GIT_TAG        v1.5.1)
FetchContent_MakeAvailable (cookmem)
```

**add_subdirectory** uses a copy of CookMem inside your project, such as a
Git submodule:

```cmake
add_subdirectory (third_party/cookmem)
```

**find_package** uses an installed copy of CookMem.  To install it:

```bash
cmake -S cookmem -B cookmem-build -DCOOKMEM_BUILD_TESTS=OFF
cmake --install cookmem-build --prefix /path/to/prefix
```

Then, in your project:

```cmake
find_package (cookmem 1.5 REQUIRED)
```

If CookMem is not installed in a standard location, pass
`-DCMAKE_PREFIX_PATH=/path/to/prefix` when you configure your project.  The
version check accepts any release with the same major version.

With FetchContent and add_subdirectory, only the `cookmem::cookmem` target is
created.  CookMem's tests, examples and performance tests are built only when
CookMem is the top-level project.  The following options change that:

* `COOKMEM_BUILD_TESTS` - build the tests, examples and performance tests.
  The default is `ON` only when CookMem is the top-level project.
* `COOKMEM_INSTALL` - generate the install rules.  The default is `ON` only
  when CookMem is the top-level project.

### Without CMake

Copy `cookmem.h` and the `cookmem/` directory from `include/` into a
suitable include directory.

### Version

The version of CookMem is available through the macros in `cookmem/version.h`,
which every CookMem header includes:

* `COOKMEM_VERSION_MAJOR`, `COOKMEM_VERSION_MINOR` and `COOKMEM_VERSION_PATCH`
* `COOKMEM_VERSION` - a single number for comparisons, such as 10203 for 1.2.3
* `COOKMEM_VERSION_STRING` - a string such as "1.2.3"

```cpp
#if COOKMEM_VERSION < 10000
#error "CookMem 1.0 or later is required."
#endif
```
