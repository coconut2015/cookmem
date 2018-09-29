#!/bin/bash
#
# Copyright (c) 2018 Heng Yuan
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
		rm -rf ../docs
		mv html ../docs
	;;
	*)
		echo "Unknown build option: $BUILD"
	exit 1
	;;
esac
