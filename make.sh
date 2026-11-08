#!/bin/bash
#
# Copyright (c) 2018-2026 Heng Yuan
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

usage ()
{
	cat <<EOF
Usage:
    ./make.sh [build]

build:
    debug               - debug build, then run the tests (default).
    cov                 - debug build with code coverage, then run the tests
                          and generate the report in cov/test_coverage.
    release             - release build, then run the tests, including the
                          performance tests.
    profiler            - release build with -pg for gprof, then run the tests.
    docs                - generate Doxygen documents into docs.
    help                - print this help message.
EOF
}

BUILD=debug

if [ $# -gt 1 ]; then
	usage
	exit 1
fi

if [ $# -eq 1 ]; then
	BUILD=$1
fi

CMAKE_BUILD_TYPE=
# Turn coverage off explicitly, since CMake caches it in the build directory.
COVERAGE=-DCOVERAGE=0
BUILD_DOC=
PROFILER=

case "$BUILD" in
	debug)
		CMAKE_BUILD_TYPE=Debug
	;;
	cov)
		CMAKE_BUILD_TYPE=Debug
		COVERAGE=-DCOVERAGE=1
	;;
	release)
		CMAKE_BUILD_TYPE=Release
	;;
	profiler)
		CMAKE_BUILD_TYPE=Release
		PROFILER="-DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg"
	;;
	doc|docs)
		BUILD=doc
		CMAKE_BUILD_TYPE=Release
		BUILD_DOC=-DBUILD_DOC=1
	;;
	help|-h|--help)
		usage
		exit 0
	;;
	*)
		echo "Unknown build option: $BUILD"
		usage
		exit 1
	;;
esac

if [ ! -d $BUILD ]; then
	mkdir $BUILD
fi

cd $BUILD

# Build in parallel, using all the processors.
JOBS=$(nproc 2>/dev/null || echo 2)

cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE $COVERAGE $PROFILER $BUILD_DOC .. || exit $?

case "$BUILD" in
	debug|release|profiler)
		make -j$JOBS && ctest
	;;
	cov)
		# test_coverage runs the tests itself, so there is no separate ctest.
		make -j$JOBS && make test_coverage
	;;
	doc)
		make doc_doxygen || exit $?
		rm -rf ../docs
		mv html ../docs
	;;
esac
