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

#include <cookmem.h>
#include <cookmmaparena.h>

#define USE_DL_PREFIX   1
#define HAVE_MORECORE   0
#include "dlmalloc.c"

static bool s_useDLMalloc = true;
static cookmem::MmapArena s_arena;
static cookmem::CachedArena<cookmem::MmapArena> s_cachedArena (s_arena);
static cookmem::MemPool<cookmem::CachedArena<cookmem::MmapArena> > s_pool (s_cachedArena);

extern "C"
{

void* malloc(std::size_t size)
{
    if (s_useDLMalloc)
    {
        return dlmalloc(size);
    }
    {
        return s_pool.allocate(size);
    }
}

void free(void* ptr)
{
    if (s_useDLMalloc)
    {
        dlfree(ptr);
    }
    else
    {
        s_pool.deallocate(ptr);
    }
}

void* calloc(std::size_t num, std::size_t size)
{
    if (s_useDLMalloc)
    {
        return dlcalloc(num, size);
    }
    else
    {
        return s_pool.callocate(num, size);
    }
}

void* realloc(void* ptr, std::size_t size)
{
    if (s_useDLMalloc)
    {
        return dlrealloc(ptr, size);
    }
    else
    {
        return s_pool.reallocate(ptr, size);
    }
}

}
