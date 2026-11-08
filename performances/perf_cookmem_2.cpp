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
 * Fill, clear and fill again.
 *
 * Same as perf_cookmem_1, but the vector is cleared and filled a second
 * time, so the second pass reuses the chunks freed by the first.
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

        strings.clear ();

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
