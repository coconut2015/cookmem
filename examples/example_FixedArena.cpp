/*
 * Copyright (c) 2018-2026 Heng Yuan
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
    // Reserve a 64,000-byte buffer for the arena to manage. The buffer lives
    // on the stack, so its storage is limited and remains valid only until
    // main returns. Keep it alive for at least as long as the arena and the
    // memory context that use it.
    char buffer[64000];
    // FixedArena manages this caller-provided buffer; it does not allocate
    // or own the buffer itself. CookMem requires 16-byte alignment, so if the
    // buffer start is not aligned, FixedArena advances it to the next
    // 16-byte boundary and reduces the usable size to account for the skipped
    // prefix. It also rounds the remaining size down to a multiple of 16.
    // Because the arena has a fixed capacity, allocations can fail when its
    // available storage is exhausted.
    cookmem::FixedArena arena (buffer, sizeof(buffer));
    // NoActionMemLogger suppresses routine allocation and deallocation logs.
    // It still reports detected memory errors by throwing an exception.
    cookmem::NoActionMemLogger logger;
    // MemContext combines the arena and logger and provides the allocation
    // interface used below. It stores references to both, so they must remain
    // alive for the lifetime of the context.
    cookmem::MemContext<cookmem::FixedArena, cookmem::NoActionMemLogger> memCtx (arena, logger);

    // Allocate at least 100 bytes. The returned pointer is null if the
    // request cannot be satisfied by the fixed buffer.
    void* ptr = memCtx.allocate (100);

    // Return this block to the memory context so its space can be reused.
    memCtx.deallocate (ptr);

    // Like C's calloc, allocate one 100-byte block and initialize it to zero.
    ptr = memCtx.callocate (1, 100);

    // Grow the block to 1,000 bytes. Reallocation preserves its existing
    // contents; if the request cannot be satisfied, it returns null and the
    // original block remains valid.
    ptr = memCtx.reallocate (ptr, 1000);

    // Release the resized block when it is no longer needed.
    memCtx.deallocate (ptr);

    return 0;
}
