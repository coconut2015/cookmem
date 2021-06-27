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
#ifndef COOK_MEM_LOGGER_H
#define COOK_MEM_LOGGER_H

#include <cstddef>
#include <cstdint>

#include "cookexception.h"

namespace cookmem
{

/**
 * A MemLogger logs memory allocation, deallocation, and errors.
 *
 * This class here is merely used to define the functions needed to implement
 * a MemLogger.
 */
class MemLogger
{
    virtual ~MemLogger () { }

    /**
     * Log the memory segment allocation.
     *
     * @param   segment
     *          the segment obtained.
     * @param   segmentSize
     *          the segment size.
     */
    virtual void
    logGetSegment (void* segment, std::size_t segmentSize) = 0;

    /**
     * Log the memory segment release.
     *
     * @param   segment
     *          the segment obtained.
     * @param   segmentSize
     *          the segment size.
     */
    virtual void
    logFreeSegment (void* segment, std::size_t segmentSize) = 0;

    /**
     * Log the memory allocation.
     *
     * @param   userPtr
     *          the user pointer allocated.
     * @param   userSize
     *          the user requested size.
     */
    virtual void
    logAllocation (void* userPtr, std::size_t userSize) = 0;

    /**
     * Log the memory reallocation.  This is only called when the new
     * size is smaller than the original size.
     *
     * @param   userPtr
     *          the user pointer allocated.
     * @param   oldUserSize
     *          the old user size.
     * @param   newUserSize
     *          the new user size.
     */
    inline void
    logReallocation (void* userPtr, std::size_t oldUserSize, std::size_t newUserSize) { }

    /**
     * Log the memory deallocation.
     *
     * @param   userPtr
     *          the user pointer allocated.
     * @param   userSize
     *          the user requested size.
     */
    virtual void
    logDeallocation (void* userPtr, std::size_t userSize) = 0;

    /**
     * Log the memory corruption.
     *
     * @param   userPtr
     *          the user pointer that had the memory corruption.
     * @param   error
     *          the type of the error encountered.
     */
    virtual void
    logError (void* userPtr, MemError_et error) = 0;
};

/**
 * A very simple MemLogger that does mostly nothing.
 */
class NoActionMemLogger
{
public:
    NoActionMemLogger ()
    {
    }

    inline void logGetSegment (void* segment, std::size_t segmentSize) { }

    inline void logFreeSegment (void* segment, std::size_t segmentSize) { }

    inline void logAllocation (void* userPtr, std::size_t userSize) { }

    inline void logReallocation (void* userPtr, std::size_t oldUserSize, std::size_t newUserSize) { }

    inline void logDeallocation (void* userPtr, std::size_t userSize) { }

    inline void
    logError (void* userPtr, MemError_et error)
    {
        throw Exception (error, "memory corruption detected.");
    }
};

}   // namespace cookmem

#endif  // COOK_MEM_LOGGER_H
