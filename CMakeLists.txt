CMAKE_MINIMUM_REQUIRED (VERSION 3.14)

# The version number is defined in include/cookmem/version.h.  Editing that
# file re-runs CMake.
set (COOKMEM_VERSION_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/cookmem/version.h")
file (READ "${COOKMEM_VERSION_HEADER}" COOKMEM_VERSION_HEADER_TEXT)
foreach (part MAJOR MINOR PATCH)
	if (NOT COOKMEM_VERSION_HEADER_TEXT MATCHES "#define COOKMEM_VERSION_${part} +([0-9]+)")
		message (FATAL_ERROR "COOKMEM_VERSION_${part} is not defined in ${COOKMEM_VERSION_HEADER}")
	endif ()
	set (COOKMEM_VERSION_${part} "${CMAKE_MATCH_1}")
endforeach ()
set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COOKMEM_VERSION_HEADER}")

project (CookMem
	VERSION ${COOKMEM_VERSION_MAJOR}.${COOKMEM_VERSION_MINOR}.${COOKMEM_VERSION_PATCH}
	LANGUAGES CXX
)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# CookMem is the top-level project when it is built by itself, rather than
# added to another project with add_subdirectory () or FetchContent.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set (COOKMEM_IS_TOP_LEVEL ON)
else ()
	set (COOKMEM_IS_TOP_LEVEL OFF)
endif ()

option (COOKMEM_BUILD_TESTS
	"Build the CookMem tests, examples and performance tests."
	${COOKMEM_IS_TOP_LEVEL})
option (COOKMEM_INSTALL
	"Generate the install rules for the CookMem headers and package files."
	${COOKMEM_IS_TOP_LEVEL})

# -- cookmem::cookmem --------------------------------------------------------
# CookMem is header-only.  Linking cookmem::cookmem adds the include
# directory and requires C++11.
add_library (cookmem INTERFACE)
add_library (cookmem::cookmem ALIAS cookmem)
target_include_directories (cookmem INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features (cookmem INTERFACE cxx_std_11)

# -- install -----------------------------------------------------------------
# Installs the headers and a package config, so that other projects can use
# find_package (cookmem).
if (COOKMEM_INSTALL)
	include (CMakePackageConfigHelpers)

	set (COOKMEM_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cookmem/cmake")

	install (TARGETS cookmem EXPORT cookmemTargets)
	install (DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
	install (EXPORT cookmemTargets
		NAMESPACE cookmem::
		DESTINATION "${COOKMEM_CMAKE_INSTALL_DIR}")

	configure_package_config_file (
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cookmemConfig.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cookmemConfig.cmake"
		INSTALL_DESTINATION "${COOKMEM_CMAKE_INSTALL_DIR}")
	# Releases with the same major version are compatible.  The headers do
	# not depend on the architecture of the project that installed them.
	write_basic_package_version_file (
		"${CMAKE_CURRENT_BINARY_DIR}/cookmemConfigVersion.cmake"
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion
		ARCH_INDEPENDENT)
	install (FILES
		"${CMAKE_CURRENT_BINARY_DIR}/cookmemConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/cookmemConfigVersion.cmake"
		DESTINATION "${COOKMEM_CMAKE_INSTALL_DIR}")
endif ()

# Everything below builds the CookMem tests, examples, performance tests and
# documentation, which projects that use CookMem do not need.
if (NOT COOKMEM_BUILD_TESTS)
	return ()
endif ()

set( CMAKE_CXX_STANDARD 11 )
enable_testing()

add_custom_target(ctest COMMAND ${CMAKE_CTEST_COMMAND})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
else (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
endif (MSVC)

if (UNIX)
	if(COVERAGE)
		include(CodeCoverage)
		APPEND_COVERAGE_COMPILER_FLAGS()
		# The test_coverage target is defined at the end of this file, after
		# the targets it depends on.
	endif(COVERAGE)
endif (UNIX)

if (VCPROFILER)
	# /PROFILE makes the executables usable with the Visual Studio profiling
	# tools.
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PROFILE")
endif (VCPROFILER)

#
# On ubuntu install doxygen and graphviz
#
if (BUILD_DOC)
	# check if Doxygen is installed
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		# set input and output files
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in)
		set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

		# request to configure the file
		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
		message("Doxygen build started")

		# note the option ALL which allows to build the docs together with the application
		add_custom_target( doc_doxygen ALL
			COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating API documentation with Doxygen"
			VERBATIM )
	else (DOXYGEN_FOUND)
		message("Doxygen need to be installed to generate the doxygen documentation")
	endif (DOXYGEN_FOUND)
endif (BUILD_DOC)

include_directories(include/)

# Debug builds define COOKMEM_DEBUG so that the internal assertions
# (COOKMEM_ASSERT) are checked.  Release builds are free of assertions.
add_compile_definitions ($<$<CONFIG:Debug>:COOKMEM_DEBUG=1>)

# -- tests -------------------------------------------------------------------

# .. test_circularlist
add_executable(test_circularlist
	tests/test_circularlist.cpp)

add_test(NAME test_circularlist
	COMMAND test_circularlist)

# .. test_rbtree
add_executable(test_rbtree
	tests/test_rbtree.cpp)

add_test(NAME test_rbtree
	COMMAND test_rbtree)

# .. test_avltree
add_executable(test_avltree
	tests/test_avltree.cpp)

add_test(NAME test_avltree
	COMMAND test_avltree)

# .. test_cookmem
add_executable(test_cookmem_01
	tests/test_cookmem_01.cpp)

add_test(NAME test_cookmem_01
	COMMAND test_cookmem_01)

add_executable(test_statslogger
	tests/test_statslogger.cpp)

add_test(NAME test_statslogger
	COMMAND test_statslogger)

add_executable(test_allocator
	tests/test_allocator.cpp)

add_test(NAME test_allocator
	COMMAND test_allocator)

add_executable(test_cookmem_02
	tests/test_cookmem_02.cpp)

add_test(NAME test_cookmem_02
	COMMAND test_cookmem_02)

add_executable(test_cookmem_03
	tests/test_cookmem_03.cpp)

add_test(NAME test_cookmem_03
	COMMAND test_cookmem_03)

add_executable(test_cookmem_04
	tests/test_cookmem_04.cpp)

add_test(NAME test_cookmem_04
	COMMAND test_cookmem_04)

add_executable(test_cookmem_error_01
	tests/test_cookmem_error_01.cpp)

add_test(NAME test_cookmem_error_01
	COMMAND test_cookmem_error_01)

# .. test_fixedarena
add_executable(test_fixedarena
	tests/test_fixedarena.cpp)

add_test(NAME test_fixedarena
	COMMAND test_fixedarena)

# .. test_cachedarena
add_executable(test_cachedarena
	tests/test_cachedarena.cpp)

add_test(NAME test_cachedarena
	COMMAND test_cachedarena)

# .. test_mmaparena
add_executable(test_mmaparena
	tests/test_mmaparena.cpp)

add_test(NAME test_mmaparena
	COMMAND test_mmaparena)

# .. test_cookmem_aligned
add_executable(test_cookmem_aligned
	tests/test_cookmem_aligned.cpp)

add_test(NAME test_cookmem_aligned
	COMMAND test_cookmem_aligned)

# .. test_version
add_executable(test_version
	tests/test_version.cpp)
# Check that the version CMake read from version.h matches the header.
target_compile_definitions(test_version PRIVATE
	COOKMEM_CMAKE_PROJECT_VERSION="${PROJECT_VERSION}")

add_test(NAME test_version
	COMMAND test_version)

# -- performance tests -------------------------------------------------------
# Until I figure out how to replace malloc / free on Windows, these tests
# can only be done on Linux.  They are skipped in code coverage builds, since
# they are not part of the coverage report and only take time.
if (UNIX AND NOT COVERAGE)
	add_executable(perf_cookmem_1
		performances/perf_cookmem_1.cpp)
	add_test(NAME perf_cookmem_1
		COMMAND perf_cookmem_1)

	add_executable(perf_cookmem_2
		performances/perf_cookmem_2.cpp)
	add_test(NAME perf_cookmem_2
		COMMAND perf_cookmem_2)

	add_executable(perf_cookmem_3
		performances/perf_cookmem_3.cpp)
	add_test(NAME perf_cookmem_3
		COMMAND perf_cookmem_3)

	add_executable(perf_cookmem_4
		performances/perf_cookmem_4.cpp)
	add_test(NAME perf_cookmem_4
		COMMAND perf_cookmem_4)

	add_executable(perf_cookmem_5
		performances/perf_cookmem_5.cpp)
	add_test(NAME perf_cookmem_5
		COMMAND perf_cookmem_5)

	add_executable(perf_cookmem_6
		performances/perf_cookmem_6.cpp)
	add_test(NAME perf_cookmem_6
		COMMAND perf_cookmem_6)

	add_executable(perf_cookmem_7
		performances/perf_cookmem_7.cpp)
	add_test(NAME perf_cookmem_7
		COMMAND perf_cookmem_7)

	add_executable(perf_cookmem_8
		performances/perf_cookmem_8.cpp)
	add_test(NAME perf_cookmem_8
		COMMAND perf_cookmem_8)

	add_executable(perf_cookmem_9
		performances/perf_cookmem_9.cpp)
	add_test(NAME perf_cookmem_9
		COMMAND perf_cookmem_9)

	add_executable(perf_cookmem_10
		performances/perf_cookmem_10.cpp)
	add_test(NAME perf_cookmem_10
		COMMAND perf_cookmem_10)

	add_executable(perf_cookmem_11
		performances/perf_cookmem_11.cpp)
	add_test(NAME perf_cookmem_11
		COMMAND perf_cookmem_11)

	add_executable(perf_cookmem_12
		performances/perf_cookmem_12.cpp)
	add_test(NAME perf_cookmem_12
		COMMAND perf_cookmem_12)
endif (UNIX AND NOT COVERAGE)

# -- examples -------------------------------------------------------
add_executable(example_FixedArena
	examples/example_FixedArena.cpp)
add_test(NAME example_FixedArena
	COMMAND example_FixedArena)

add_executable(example_SimpleMemContext
	examples/example_SimpleMemContext.cpp)
add_test(NAME example_SimpleMemContext
	COMMAND example_SimpleMemContext)

add_executable(example_CachedArena
	examples/example_CachedArena.cpp)
add_test(NAME example_CachedArena
	COMMAND example_CachedArena)

add_executable(example_child
	examples/example_child.cpp)
add_test(NAME example_child
	COMMAND example_child)

add_executable(example_getSize
	examples/example_getSize.cpp)
add_test(NAME example_getSize
	COMMAND example_getSize)

add_executable(example_footprintLimit
	examples/example_footprintLimit.cpp)
add_test(NAME example_footprintLimit
	COMMAND example_footprintLimit)

add_executable(example_StatsLogger
	examples/example_StatsLogger.cpp)
add_test(NAME example_StatsLogger
	COMMAND example_StatsLogger)

add_executable(example_boundcheck
	examples/example_boundcheck.cpp)
add_test(NAME example_boundcheck
	COMMAND example_boundcheck)

add_executable(example_Allocator
	examples/example_Allocator.cpp)
add_test(NAME example_Allocator
	COMMAND example_Allocator)

add_executable(example_AllocatorThreadLocal
	examples/example_AllocatorThreadLocal.cpp)
find_package(Threads REQUIRED)
target_link_libraries(example_AllocatorThreadLocal Threads::Threads)
add_test(NAME example_AllocatorThreadLocal
	COMMAND example_AllocatorThreadLocal)

# -- code coverage -----------------------------------------------------------
# test_coverage builds the test and example executables, runs the tests once
# with ctest and generates the report.
if (UNIX AND COVERAGE)
	get_property (COOKMEM_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
	set (COOKMEM_COVERAGE_DEPENDENCIES)
	foreach (target ${COOKMEM_TARGETS})
		get_target_property (target_type ${target} TYPE)
		get_target_property (target_excluded ${target} EXCLUDE_FROM_ALL)
		if (target_type STREQUAL "EXECUTABLE" AND NOT target_excluded)
			list (APPEND COOKMEM_COVERAGE_DEPENDENCIES ${target})
		endif ()
	endforeach ()

	setup_target_for_coverage_lcov (
		NAME test_coverage
		EXECUTABLE ctest
		DEPENDENCIES ${COOKMEM_COVERAGE_DEPENDENCIES}
		# The performance tests are not built in coverage builds, and lcov
		# reports an exclude pattern that matches nothing as an error.
		EXCLUDE "/usr/*" "tests/*" "examples/*"
	)
endif ()
