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

#include "malloc.h"

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

static void
test1 ()
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

int
main (int argc, const char* argv[])
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    s_useDLMalloc = true;
    test1 ();

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    s_useDLMalloc = false;
    test1 ();

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() << ","
              << std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2).count() << std::endl;
    return 0;
}
