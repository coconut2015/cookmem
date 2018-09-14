CMAKE_MINIMUM_REQUIRED (VERSION 3.1)
project (CookMem
	VERSION 0.0.0.1
	LANGUAGES CXX
)

set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_CXX_STANDARD 11 )
enable_testing()

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

add_custom_target(ctest COMMAND ${CMAKE_CTEST_COMMAND})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(COVERAGE)
	if(CMAKE_COMPILER_IS_GNUCXX)
		include(CodeCoverage)
	
		setup_target_for_coverage (
			NAME test_coverage
			EXECUTABLE ctest
			DEPENDENCIES ctest
		)
	
	endif()
	APPEND_COVERAGE_COMPILER_FLAGS()
endif(COVERAGE)

#
# On ubuntu install doxygen and graphviz
#
if(BUILD_DOC)
	# check if Doxygen is installed
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		# set input and output files
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
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
endif(BUILD_DOC)

include_directories(include/)

#add_library(cookmem STATIC
#)

# -- tests -------------------------------------------------------------------
# .. test_cookavltree
add_executable(test_cookavltree
	test/test_cookavltree.cpp)

add_test(NAME test_cookavltree
    COMMAND test_cookavltree)

# .. test_cooklru
add_executable(test_cooklru
	test/test_cooklru.cpp)

add_test(NAME test_cooklru
    COMMAND test_cooklru)

# .. test_cookptrcircularlist
add_executable(test_cookptrcircularlist
	test/test_cookptrcircularlist.cpp)

add_test(NAME test_cookptrcircularlist
    COMMAND test_cookptrcircularlist)

# .. test_cookptravltree
add_executable(test_cookptravltree
	test/test_cookptravltree.cpp)

add_test(NAME test_cookptravltree
    COMMAND test_cookptravltree)

# .. test_cookmem
add_executable(test_cookmem
	test/test_cookmem.cpp)

add_test(NAME test_cookmem
    COMMAND test_cookmem)

# .. test_fixedarena
add_executable(test_fixedarena
	test/test_fixedarena.cpp)

add_test(NAME test_fixedarena
    COMMAND test_fixedarena)

# .. test_mmaparena
add_executable(test_mmaparena
	test/test_mmaparena.cpp)

add_test(NAME test_mmaparena
    COMMAND test_mmaparena)

# .. test_cachedarena
add_executable(test_cachedarena
	test/test_cachedarena.cpp)

add_test(NAME test_cachedarena
    COMMAND test_cachedarena)

# -- performance tests -------------------------------------------------------
# .. perf_cookmem

add_executable(perf_cookmem
	performance/perf_cookmem.cpp)

add_test(NAME perf_cookmem
    COMMAND perf_cookmem)
