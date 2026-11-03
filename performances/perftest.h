/*
 * Copyright (c) 2025-2026 Heng Yuan
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
#ifndef PERF_TEST_H
#define PERF_TEST_H

#include "malloc.h"

class PerformanceTest
{
public:
    template<class T>
    int
    run (T& test)
    {
        s_mallocType = MALLOC_DLMALLOC;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now ();

        test.run ();

        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now ();

        s_mallocType = MALLOC_COOKMEM_RB;
        test.run ();

        s_mallocType = MALLOC_DLMALLOC;
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now ();

        s_mallocType = MALLOC_COOKMEM_AVL;
        test.run ();

        s_mallocType = MALLOC_DLMALLOC;
        std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now ();

        std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count () << ","
                  << std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2).count () << ","
                  << std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t3).count () << std::endl;
        return 0;
    }
};

#endif /* PERF_TEST_H */
