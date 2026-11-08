/*
 * Copyright (c) 2018 Heng Yuan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>

#include "perftest.h"

/**
 * Strings of growing length.
 *
 * 10000 strings are created, each one character longer than the previous
 * one (1 to 10000 characters).  Most of the allocations come from the
 * tree bins.
 */
struct Test1
{
    void
    run ()
    {
        std::vector<std::string> strings;
        std::string prev;

        for (int i = 0; i < 10000; ++i)
        {
            prev += 'a';
            strings.push_back(prev);
        }

        strings.clear();
        prev.clear();
    }
};

int
main (int argc, const char* argv[])
{
    Test1 test1;
    return PerformanceTest ().run (test1);
}
