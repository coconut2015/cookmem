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
 * Interleaved realloc growth.
 *
 * A set of buffers grows in round-robin order, 64 bytes at a time, up to
 * 32KB each.  Because neighboring buffers are interleaved, most of the
 * reallocations cannot be extended in place.
 */
struct Test1
{
    void
    run ()
    {
        const int count = 1000;
        const std::size_t maxSize = 32768;
        std::vector<void*> ptrs (count, nullptr);

        for (std::size_t size = 64; size <= maxSize; size += 64)
        {
            for (int i = 0; i < count; ++i)
            {
                ptrs[i] = realloc (ptrs[i], size);
                ((char*)ptrs[i])[size - 1] = (char)i;
            }
        }

        for (int i = 0; i < count; ++i)
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
