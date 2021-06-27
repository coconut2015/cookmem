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

#include <cookmem.h>

#define USE_DL_PREFIX   1
#define HAVE_MORECORE   0
#include "dlmalloc.c"

static bool s_useDLMalloc = true;
static cookmem::CachedMemContext<> s_memCtx;

extern "C"
{

void*
malloc (std::size_t size)
{
    if (s_useDLMalloc)
    {
        return dlmalloc(size);
    }
    {
        return s_memCtx.allocate(size);
    }
}

void
free (void* ptr)
{
    if (s_useDLMalloc)
    {
        dlfree(ptr);
    }
    else
    {
        s_memCtx.deallocate(ptr);
    }
}

void*
calloc (std::size_t num, std::size_t size)
{
    if (s_useDLMalloc)
    {
        return dlcalloc(num, size);
    }
    else
    {
        return s_memCtx.callocate(num, size);
    }
}

void*
realloc (void* ptr, std::size_t size)
{
    if (s_useDLMalloc)
    {
        return dlrealloc(ptr, size);
    }
    else
    {
        return s_memCtx.reallocate(ptr, size);
    }
}

}
