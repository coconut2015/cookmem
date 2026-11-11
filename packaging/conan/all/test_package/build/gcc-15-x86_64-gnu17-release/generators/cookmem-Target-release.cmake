# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(cookmem_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(cookmem_FRAMEWORKS_FOUND_RELEASE "${cookmem_FRAMEWORKS_RELEASE}" "${cookmem_FRAMEWORK_DIRS_RELEASE}")

set(cookmem_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET cookmem_DEPS_TARGET)
    add_library(cookmem_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET cookmem_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${cookmem_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${cookmem_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### cookmem_DEPS_TARGET to all of them
conan_package_library_targets("${cookmem_LIBS_RELEASE}"    # libraries
                              "${cookmem_LIB_DIRS_RELEASE}" # package_libdir
                              "${cookmem_BIN_DIRS_RELEASE}" # package_bindir
                              "${cookmem_LIBRARY_TYPE_RELEASE}"
                              "${cookmem_IS_HOST_WINDOWS_RELEASE}"
                              cookmem_DEPS_TARGET
                              cookmem_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "cookmem"    # package_name
                              "${cookmem_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${cookmem_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET cookmem::cookmem
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${cookmem_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${cookmem_LIBRARIES_TARGETS}>
                 )

    if("${cookmem_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET cookmem::cookmem
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     cookmem_DEPS_TARGET)
    endif()

    set_property(TARGET cookmem::cookmem
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${cookmem_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET cookmem::cookmem
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${cookmem_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET cookmem::cookmem
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${cookmem_LIB_DIRS_RELEASE}>)
    set_property(TARGET cookmem::cookmem
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${cookmem_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET cookmem::cookmem
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${cookmem_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(cookmem_LIBRARIES_RELEASE cookmem::cookmem)
