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
 * Large allocations.
 *
 * Blocks between 128KB and about 4MB are allocated and freed, with up to
 * 8 blocks alive at any time.  This exercises segment allocation and
 * release in the arena.
 */
struct Test1
{
    void
    run ()
    {
        const int live = 8;
        void* ptrs[live] = { nullptr };

        for (int i = 0; i < 4000; ++i)
        {
            int slot = i % live;
            free (ptrs[slot]);

            std::size_t size = 131072 + ((std::size_t)i * 104729) % (4 * 1024 * 1024);
            ptrs[slot] = malloc (size);
            *(char*)ptrs[slot] = (char)i;
        }

        for (int i = 0; i < live; ++i)
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
