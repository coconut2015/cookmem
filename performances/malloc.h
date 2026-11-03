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
#ifndef MALLOC_H
#define MALLOC_H

#include <cookmem.h>

#define USE_DL_PREFIX   1
#define HAVE_MORECORE   0
#include "dlmalloc.c"

typedef enum
{
    MALLOC_DLMALLOC,
    MALLOC_COOKMEM_RB,
    MALLOC_COOKMEM_AVL
} MALLOC_TYPE_et;

static MALLOC_TYPE_et s_mallocType = MALLOC_DLMALLOC;
static cookmem::CachedMemContext<> s_memCtx_rb;
static cookmem::CachedMemContext<cookmem::MmapArena, cookmem::NoActionMemLogger, cookmem::PtrAVLTree> s_memCtx_avl;

extern "C"
{

void*
malloc (std::size_t size)
{
    if (s_mallocType == MALLOC_DLMALLOC)
    {
        return dlmalloc (size);
    }
    else if (s_mallocType == MALLOC_COOKMEM_RB)
    {
        return s_memCtx_rb.allocate (size);
    }
    else
    {
        return s_memCtx_avl.allocate (size);
    }
}

void
free (void* ptr)
{
    if (s_mallocType == MALLOC_DLMALLOC)
    {
        dlfree (ptr);
    }
    else if (s_mallocType == MALLOC_COOKMEM_RB)
    {
        return s_memCtx_rb.deallocate (ptr);
    }
    else
    {
        return s_memCtx_avl.deallocate (ptr);
    }
}

void*
calloc (std::size_t num, std::size_t size)
{
    if (s_mallocType == MALLOC_DLMALLOC)
    {
        return dlcalloc (num, size);
    }
    else if (s_mallocType == MALLOC_COOKMEM_RB)
    {
        return s_memCtx_rb.callocate (num, size);
    }
    else
    {
        return s_memCtx_avl.callocate (num, size);
    }
}

void*
realloc (void* ptr, std::size_t size)
{
    if (s_mallocType == MALLOC_DLMALLOC)
    {
        return dlrealloc (ptr, size);
    }
    else if (s_mallocType == MALLOC_COOKMEM_RB)
    {
        return s_memCtx_rb.reallocate (ptr, size);
    }
    else
    {
        return s_memCtx_avl.reallocate (ptr, size);
    }
}

}

#endif      /* MALLOC_H */
