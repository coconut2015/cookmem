/*
 * Copyright (c) 2026 Heng Yuan
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
 * Many growing vectors.
 *
 * Each std::vector grows one element at a time, so its storage is
 * reallocated geometrically through new / delete.  All vectors stay alive
 * until the end, and they are built in an interleaved order.
 */
struct Test1
{
    void
    run ()
    {
        const int count = 10000;
        std::vector<std::vector<int>> vectors (count);

        for (int n = 0; n < 500; ++n)
        {
            for (int i = 0; i < count; ++i)
            {
                vectors[i].push_back (n);
            }
        }

        vectors.clear ();
    }
};

int
main (int argc, const char* argv[])
{
    Test1 test1;
    return PerformanceTest ().run (test1);
}
