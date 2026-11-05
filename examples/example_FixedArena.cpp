/*
 * Copyright (c) 2018-2021 Heng Yuan
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
#include <iostream>

#include <cookmem.h>

int
main (int argc, const char* argv[])
{
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));
    cookmem::NoActionMemLogger logger;
    cookmem::MemContext<cookmem::FixedArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

    // Allocate memory
    void* ptr = memCtx.allocate (100);

    // Free the memory
    memCtx.deallocate (ptr);

    // Much like calloc in c, allocate 100 bytes which is set to 0.
    ptr = memCtx.callocate (1, 100);

    // Change the size of the memory
    ptr = memCtx.reallocate (ptr, 1000);

    // Free the memory
    memCtx.deallocate (ptr);

    return 0;
}
