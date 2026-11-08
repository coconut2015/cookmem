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
 * Sliding window (FIFO) of allocations.
 *
 * A fixed number of blocks stays alive.  On each step the oldest block is
 * freed and a new one is allocated, with sizes cycling from 16 to 2047
 * bytes.  This resembles a queue of messages or requests.
 */
struct Test1
{
    void
    run ()
    {
        const int window = 1024;
        std::vector<void*> ptrs (window, nullptr);

        for (int i = 0; i < 2000000; ++i)
        {
            int slot = i % window;
            free (ptrs[slot]);

            ptrs[slot] = malloc (16 + (i * 131) % 2032);
            *(char*)ptrs[slot] = (char)i;
        }

        for (int i = 0; i < window; ++i)
        {
            free (ptrs[i]);
        }
    }
};

int
main (int argc, const char* argv[])
{
    Test1 test1;
    return PerformanceTest ().run (test1);
}
