/*
 * Copyright (c) 2018-2026 Heng Yuan
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
 * Many copies of the same string.
 *
 * A 200-character string is copied into a std::vector one million times.
 * Each string buffer comes from the small bins, while the vector storage
 * grows into large blocks.
 */
struct Test1
{
    void
    run ()
    {
        std::vector<std::string> strings;

        std::string str("01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

        for (int i = 0; i < 1000000; ++i)
        {
            strings.push_back(str);
        }
    }
};

int
main (int argc, const char* argv[])
{
    Test1 test1;
    return PerformanceTest ().run (test1);
}
