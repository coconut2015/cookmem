# CookMem is header-only.
set(VCPKG_BUILD_TYPE release)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO coconut2015/cookmem
    REF "v${VERSION}"
    SHA512 536fc8fd24e75a8bc42ca6647f2c958822e2c9208accd3c08115a8898efa283a1f7118753c8905623cfb050703e24c8c7cfcc74eaf54db22b1ed29c8c688f84c
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
