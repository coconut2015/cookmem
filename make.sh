#!/bin/bash

BUILD=debug

if [ $# -gt 0 ]; then
	BUILD=$1
fi

CMAKE_BUILD_TYPE=
COVERAGE=
BUILD_DOC=
PROFILER=

case "$BUILD" in
	release)
		CMAKE_BUILD_TYPE=Release
	;;
	debug)
		CMAKE_BUILD_TYPE=Debug
		COVERAGE=-DCOVERAGE=1
	;;
	profiler)
		CMAKE_BUILD_TYPE=Release
		PROFILER="-DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg"
	;;
	doc)
		CMAKE_BUILD_TYPE=Release
		BUILD_DOC=-DBUILD_DOC=1
	;;
	*)
		echo "Unknown build option: $BUILD"
	exit 1
	;;
esac

if [ ! -d $BUILD ]; then
	mkdir $BUILD
fi

cd $BUILD

cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE $COVERAGE $PROFILER $BUILD_DOC ..|| exit $?

case "$BUILD" in
	release)
		make VERBOSE=1 && make test
	;;
	debug)
		make && make test_coverage
	;;
	profiler)
		make VERBOSE=1 && make test
	;;
	doc)
		make doc_doxygen
	;;
	*)
		echo "Unknown build option: $BUILD"
	exit 1
	;;
esac
