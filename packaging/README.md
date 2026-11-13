Packaging
=========

Recipes for publishing CookMem to package managers.  Each directory uses the
layout of the registry it is submitted to, so it can be copied there as is.

vcpkg
-----

`vcpkg/ports/cookmem` is a port for the
[vcpkg registry](https://github.com/microsoft/vcpkg).  To test it with a local
vcpkg checkout:

```bash
vcpkg install cookmem --overlay-ports=packaging/vcpkg/ports
```

To submit it, copy the directory to `ports/cookmem` in a vcpkg fork, then run
`vcpkg x-add-version cookmem` and open a pull request.

Conan
-----

`conan/` is a recipe in the layout of
[ConanCenter](https://github.com/conan-io/conan-center-index), where it goes in
`recipes/cookmem`.  To test it:

```bash
conan create packaging/conan/all --version 1.6.0
```

This builds and runs `test_package`, which links `cookmem::cookmem`.

Releasing a new version
-----------------------

1. Download the release archive from
   `https://github.com/coconut2015/cookmem/archive/refs/tags/vX.Y.Z.tar.gz`.
2. vcpkg: update `version` in `vcpkg.json` and `SHA512` in `portfile.cmake`
   (`sha512sum`).
3. Conan: add the version to `conan/config.yml` and its URL and `sha256` to
   `conan/all/conandata.yml`.
