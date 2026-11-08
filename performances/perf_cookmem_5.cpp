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
#include <list>

#include "perftest.h"

/**
 * Small fixed-size allocation churn.
 *
 * Every node of std::list<int> has the same small size, so this test
 * exercises the small bins only.
 */
struct Test1
{
    void
    run ()
    {
        std::list<int> numbers;

        for (int round = 0; round < 3; ++round)
        {
            for (int i = 0; i < 1000000; ++i)
            {
                numbers.push_back (i);
            }

            while (!numbers.empty ())
            {
                numbers.pop_front ();
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
