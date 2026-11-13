# CookMem is header-only.
set(VCPKG_BUILD_TYPE release)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO coconut2015/cookmem
    REF "v${VERSION}"
    SHA512 b4a5dd2d333ecebbd8c1fa622933d82d1feb0a1d91aa8e4ecfaad63a9ad68ab27e72c3c6001d7c2bb0809eb33c58e3b17a796fcc6f6fcf772d92bbefa1d5621b
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCOOKMEM_BUILD_TESTS=OFF
        -DCOOKMEM_INSTALL=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH share/cookmem/cmake)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
