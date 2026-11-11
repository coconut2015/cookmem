# CookMem is header-only.
set(VCPKG_BUILD_TYPE release)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO coconut2015/cookmem
    REF "v${VERSION}"
    SHA512 3df42532493eb0b4f6a69eb487113a476420ce0d2a320e9629f9f37877223e7e84196d59d4054d849a0af62e1da22ddf7e486e200a1e7f70698356d0844ddf5c
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
