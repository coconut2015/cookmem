Installation {#install}
------------

CookMem is a collection of C++ header files, so there are no libraries to
install. Put `cookmem.h` and the `cookmem/` directory in a suitable include
directory, then include `cookmem.h` in your code.

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
