/*
 * Copyright (c) 2021 Heng Yuan
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
#ifndef COOK_MALLOC_MEM_ARENA_H
#define COOK_MALLOC_MEM_ARENA_H

#include <cookmemarena.h>

#include <cstdlib>

namespace cookmem
{

/**
 * An malloc/free based memory arena.
 */
class MallocArena
{
public:
    /**
     * Constructor.
     *
     * @param   minSize
     *          minimum segment size.  It should be noted that this value
     *          needs to be a multiple of 16.
     */
    MallocArena (std::size_t minSize = 65536)
    : m_minSize (minSize)
    {
    }

    /**
     * Allocate an arena segment using malloc.
     *
     * @param   size
     *          the size of the segment.  This value is updated upon successful
     *          request to indicate the actual size obtained.
     * @return  the allocated pointer.  nullptr is allocation failed.
     */
    void*
    getSegment (std::size_t& size)
    {
        if (size < m_minSize)
        {
            size = m_minSize;
        }
        return malloc (size);
    }

    /**
     * Free an arena segment using free.
     *
     * @param   ptr
     *          the pointer to be freed.
     * @param   size
     *          the size of the pointer.
     * @return  true if there is an error.  false is okay.
     */
    bool
    freeSegment (void* ptr, std::size_t size)
    {
        free (ptr);
        return false;
    }

private:
    std::size_t m_minSize;
};

}   // namespace cookmem

#endif  // COOK_MALLOC_MEM_ARENA_H
