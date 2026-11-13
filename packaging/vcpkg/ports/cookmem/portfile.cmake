# CookMem is header-only.
set(VCPKG_BUILD_TYPE release)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO coconut2015/cookmem
    REF "v${VERSION}"
    SHA512 92ed2da3b23f693e20a0d267192638f8b09b3c3fb787d68a8a052714782959053caaa4e390051ee87bef5cc58eb80889b28d3431db9462a330d4d89cdbd4feab
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
