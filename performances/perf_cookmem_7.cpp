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
 * Many free chunks of the same large size.
 *
 * Freeing every other block leaves a large number of free chunks of
 * exactly the same size in one tree bin.  Freeing the remaining blocks
 * then merges each of them with its free neighbors, which removes those
 * neighbors from the tree by pointer.
 */
struct Test1
{
    void
    run ()
    {
        const int count = 50000;
        std::vector<void*> ptrs (count);

        for (int round = 0; round < 2; ++round)
        {
            for (int i = 0; i < count; ++i)
            {
                ptrs[i] = malloc (1024);
                *(char*)ptrs[i] = (char)i;
            }

            for (int i = 0; i < count; i += 2)
            {
                free (ptrs[i]);
            }

            for (int i = 1; i < count; i += 2)
            {
                free (ptrs[i]);
            }
        }
    }
};

int
main (int argc, const char* argv[])
{
    Test1 test1;
    return PerformanceTest ().run (test1);
}
