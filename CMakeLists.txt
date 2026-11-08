CMAKE_MINIMUM_REQUIRED (VERSION 3.12)

# The version number is kept in version.txt.  Editing it re-runs CMake.
file (STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" COOKMEM_VERSION LIMIT_COUNT 1)
string (STRIP "${COOKMEM_VERSION}" COOKMEM_VERSION)
set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt")

project (CookMem
	VERSION ${COOKMEM_VERSION}
	LANGUAGES CXX
)

set( CMAKE_CXX_STANDARD 11 )
enable_testing()

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

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

		setup_target_for_coverage_lcov (
			NAME test_coverage
			EXECUTABLE ctest
			DEPENDENCIES ctest
			EXCLUDE "/usr/*" "tests/*" "examples/*" "performances/*"
		)

		APPEND_COVERAGE_COMPILER_FLAGS()
	endif(COVERAGE)
endif (UNIX)

if (VCPROFILER)
	set(LINK_OPTIONS "/PROFILE")
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

#add_library(cookmem STATIC
#)

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

# -- performance tests -------------------------------------------------------
# Until I figure out how to replace malloc / free on Windows, these tests
# can only be done on Linux.
if (UNIX)
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
endif (UNIX)

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
add_test(NAME example_AllocatorThreadLocal
	COMMAND example_AllocatorThreadLocal)
