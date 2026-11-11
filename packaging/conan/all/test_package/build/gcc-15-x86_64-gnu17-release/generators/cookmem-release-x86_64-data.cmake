########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(cookmem_COMPONENT_NAMES "")
if(DEFINED cookmem_FIND_DEPENDENCY_NAMES)
  list(APPEND cookmem_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES cookmem_FIND_DEPENDENCY_NAMES)
else()
  set(cookmem_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(cookmem_PACKAGE_FOLDER_RELEASE "/tmp/claude-1000/-mnt-d-workspace-cookmem/be9d1e08-2de2-4c9a-a8a2-13ba2fa064f7/scratchpad/conanhome/p/b/cookme1d551f7ae931/p")
set(cookmem_BUILD_MODULES_PATHS_RELEASE )


set(cookmem_INCLUDE_DIRS_RELEASE "${cookmem_PACKAGE_FOLDER_RELEASE}/include")
set(cookmem_RES_DIRS_RELEASE )
set(cookmem_DEFINITIONS_RELEASE )
set(cookmem_SHARED_LINK_FLAGS_RELEASE )
set(cookmem_EXE_LINK_FLAGS_RELEASE )
set(cookmem_OBJECTS_RELEASE )
set(cookmem_COMPILE_DEFINITIONS_RELEASE )
set(cookmem_COMPILE_OPTIONS_C_RELEASE )
set(cookmem_COMPILE_OPTIONS_CXX_RELEASE )
set(cookmem_LIB_DIRS_RELEASE )
set(cookmem_BIN_DIRS_RELEASE )
set(cookmem_LIBRARY_TYPE_RELEASE UNKNOWN)
set(cookmem_IS_HOST_WINDOWS_RELEASE 0)
set(cookmem_LIBS_RELEASE )
set(cookmem_SYSTEM_LIBS_RELEASE )
set(cookmem_FRAMEWORK_DIRS_RELEASE )
set(cookmem_FRAMEWORKS_RELEASE )
set(cookmem_BUILD_DIRS_RELEASE )
set(cookmem_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(cookmem_COMPILE_OPTIONS_RELEASE)
if (NOT "${cookmem_COMPILE_OPTIONS_CXX_RELEASE}" STREQUAL "")
    list(APPEND cookmem_COMPILE_OPTIONS_RELEASE
        "$<$<COMPILE_LANGUAGE:CXX>:${cookmem_COMPILE_OPTIONS_CXX_RELEASE}>")
endif ()
if (NOT "${cookmem_COMPILE_OPTIONS_C_RELEASE}" STREQUAL "")
    list(APPEND cookmem_COMPILE_OPTIONS_RELEASE
        "$<$<COMPILE_LANGUAGE:C>:${cookmem_COMPILE_OPTIONS_C_RELEASE}>")
endif ()
set(cookmem_LINKER_FLAGS_RELEASE)
if (NOT "${cookmem_SHARED_LINK_FLAGS_RELEASE}" STREQUAL "")
    list(APPEND cookmem_LINKER_FLAGS_RELEASE
        "$<$<OR:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>,$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>>:${cookmem_SHARED_LINK_FLAGS_RELEASE}>")
endif ()
if (NOT "${cookmem_EXE_LINK_FLAGS_RELEASE}" STREQUAL "")
    list(APPEND cookmem_LINKER_FLAGS_RELEASE
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cookmem_EXE_LINK_FLAGS_RELEASE}>")
endif ()


set(cookmem_COMPONENTS_RELEASE )