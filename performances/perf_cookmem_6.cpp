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
#include <map>

#include "perftest.h"

/**
 * Mixed-size workload.
 *
 * A std::map with string values of 0 to 999 characters.  The map nodes
 * come from the small bins while most of the strings come from the tree
 * bins.  Half of the entries are erased and then re-inserted with
 * different lengths.
 */
struct Test1
{
    void
    run ()
    {
        std::map<int, std::string> table;
        const int count = 100000;

        for (int i = 0; i < count; ++i)
        {
            int key = (i * 7919) % count;
            table[key] = std::string ((i * 37) % 1000, 'a');
        }

        for (int i = 0; i < count; i += 2)
        {
            table.erase ((i * 7919) % count);
        }

        for (int i = 0; i < count; i += 2)
        {
            int key = (i * 7919) % count;
            table[key] = std::string ((i * 53) % 1000, 'b');
        }

        table.clear ();
    }
};

int
main (int argc, const char* argv[])
{
    Test1 test1;
    return PerformanceTest ().run (test1);
}
