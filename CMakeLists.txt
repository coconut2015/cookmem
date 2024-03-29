CMAKE_MINIMUM_REQUIRED (VERSION 3.1)
project (CookMem
	VERSION 0.0.0.1
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

#add_library(cookmem STATIC
#)

# -- tests -------------------------------------------------------------------

# .. test_cookptrcircularlist
add_executable(test_cookptrcircularlist
	tests/test_cookptrcircularlist.cpp)

add_test(NAME test_cookptrcircularlist
	COMMAND test_cookptrcircularlist)

# .. test_cookptravltree
add_executable(test_cookptravltree
	tests/test_cookptravltree.cpp)

add_test(NAME test_cookptravltree
	COMMAND test_cookptravltree)

# .. test_cookmem
add_executable(test_cookmem_01
	tests/test_cookmem_01.cpp)

add_test(NAME test_cookmem_01
	COMMAND test_cookmem_01)

add_executable(test_cookmem_02
	tests/test_cookmem_02.cpp)

add_test(NAME test_cookmem_02
	COMMAND test_cookmem_02)

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
endif (UNIX)

# -- examples -------------------------------------------------------
add_executable(ex_1
	examples/ex_1.cpp)
add_test(NAME ex_1
	COMMAND ex_1)

add_executable(ex_2
	examples/ex_2.cpp)
add_test(NAME ex_2
	COMMAND ex_2)

add_executable(ex_3
	examples/ex_3.cpp)
add_test(NAME ex_3
	COMMAND ex_3)

add_executable(ex_4
	examples/ex_4.cpp)
add_test(NAME ex_4
	COMMAND ex_4)

add_executable(ex_5
	examples/ex_5.cpp)
add_test(NAME ex_5
	COMMAND ex_5)

add_executable(ex_6
	examples/ex_6.cpp)
add_test(NAME ex_6
	COMMAND ex_6)
