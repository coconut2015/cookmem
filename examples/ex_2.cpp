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
    // Use default arena
    cookmem::SimpleMemContext<> memCtx;

    // allocate memory
    void* ptr = memCtx.allocate (100);

    // change the size of the memory
    ptr = memCtx.reallocate (ptr, 1000);

    // free the memory
    memCtx.deallocate (ptr);

    return 0;
}
