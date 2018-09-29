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
#include <iostream>
#include <cookmem.h>

int main(int argc, const char* argv[])
{
    char buffer[64000];
    cookmem::FixedArena arena (buffer, sizeof(buffer));
    cookmem::MemPool<cookmem::FixedArena> pool (arena);

    // allocate memory
    void* ptr = pool.allocate(100);

    // free the memory
    pool.deallocate(ptr);

    // much like calloc in c, allocate 100 bytes which is set to 0.
    ptr = pool.callocate(1, 100);

    // change the size of the memory
    ptr = pool.reallocate(ptr, 1000);

    // free the memory
    pool.deallocate(ptr);

    return 0;
}
