/*
 * Copyright (c) 2021-2026 Heng Yuan
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

#include "exception.h"

namespace cookmem
{

typedef enum
{
    /** No allocation */
    MEM_ALLOC_NONE,
    /** Small allocation using small bins */
    MEM_ALLOC_SMALL,
    /** Large allocation using tree bins */
    MEM_ALLOC_TREE,
    /** Designated victim allocation */
    MEM_ALLOC_DV,
    /** Allocation obtained from a new arena segment */
    MEM_ALLOC_ARENA,
    /** Big allocation */
    MEM_ALLOC_BIG,

    /**
     * Simply used to count the number of alloc types.
     * All new types should be added before this one.
     */
    MEM_ALLOC_COUNT
} MemAllocType_et;

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
     * @param   allocType
     *          the type of the allocation.
     */
    virtual void
    logAllocation (void* userPtr, std::size_t userSize, MemAllocType_et allocType) = 0;

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
    virtual void
    logReallocation (void* userPtr, std::size_t oldUserSize, std::size_t newUserSize) = 0;

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
    logError (void* userPtr, MemError_et error, const char* file = __FILE__, int line = __LINE__) = 0;
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

    inline void logAllocation (void* userPtr, std::size_t userSize, MemAllocType_et allocType) { }

    inline void logReallocation (void* userPtr, std::size_t oldUserSize, std::size_t newUserSize) { }

    inline void logDeallocation (void* userPtr, std::size_t userSize) { }

    inline void
    logError (void* userPtr,
              MemError_et error,
              const char* file = __FILE__,
              int line = __LINE__)
    {
        throw Exception (error, "memory corruption detected.", file, line);
    }
};

/**
 * A MemLogger that tracks allocation and segment usage statistics.
 */
class StatsLogger
{
public:
    StatsLogger ()
    : m_numAllocations (0),
      m_numDeallocations (0),
      m_numGetSegments (0),
      m_numFreeSegments (0),
      m_totalAllocated (0),
      m_totalDeallocated (0),
      m_totalGetSegmentSize (0),
      m_totalFreeSegmentSize (0),
      m_numAllocationsByType {}
    {
    }

    void
    logGetSegment (void*, std::size_t segmentSize)
    {
        ++m_numGetSegments;
        m_totalGetSegmentSize += segmentSize;
    }

    void
    logFreeSegment (void*, std::size_t segmentSize)
    {
        ++m_numFreeSegments;
        m_totalFreeSegmentSize += segmentSize;
    }

    void
    logAllocation (void* userPtr, std::size_t userSize, MemAllocType_et allocType)
    {
        if (userPtr != nullptr)
        {
            ++m_numAllocations;
            m_totalAllocated += userSize;
            ++m_numAllocationsByType[allocType];
        }
    }

    void
    logReallocation (void*, std::size_t oldUserSize, std::size_t newUserSize)
    {
        if (newUserSize > oldUserSize)
        {
            m_totalAllocated += newUserSize - oldUserSize;
        }
        else
        {
            m_totalDeallocated += oldUserSize - newUserSize;
        }
    }

    void
    logDeallocation (void* userPtr, std::size_t userSize)
    {
        if (userPtr != nullptr)
        {
            ++m_numDeallocations;
            m_totalDeallocated += userSize;
        }
    }

    void
    logError (void*,
              MemError_et error,
              const char* file = __FILE__,
              int line = __LINE__)
    {
        throw Exception (error, "memory corruption detected.", file, line);
    }

    /** Get the number of successful allocation events. */
    std::size_t getNumAllocations () const { return m_numAllocations; }
    /** Get the number of successful allocations of a particular type. */
    std::size_t getNumAllocations (MemAllocType_et allocType) const
    {
        return m_numAllocationsByType[allocType];
    }
    /** Get the number of deallocation events. */
    std::size_t getNumDeallocations () const { return m_numDeallocations; }
    /** Get the number of acquired memory segments. */
    std::size_t getNumGetSegments () const { return m_numGetSegments; }
    /** Get the number of released memory segments. */
    std::size_t getNumFreeSegments () const { return m_numFreeSegments; }
    /** Get the total number of bytes allocated. */
    std::size_t getTotalAllocated () const { return m_totalAllocated; }
    /** Get the total number of bytes deallocated. */
    std::size_t getTotalDeallocated () const { return m_totalDeallocated; }
    /** Get the total size of acquired memory segments. */
    std::size_t getTotalGetSegmentSize () const { return m_totalGetSegmentSize; }
    /** Get the total size of released memory segments. */
    std::size_t getTotalFreeSegmentSize () const { return m_totalFreeSegmentSize; }

    void
    printStats ()
    {

    }

private:
    std::size_t m_numAllocations;
    std::size_t m_numDeallocations;
    std::size_t m_numGetSegments;
    std::size_t m_numFreeSegments;
    std::size_t m_totalAllocated;
    std::size_t m_totalDeallocated;
    std::size_t m_totalGetSegmentSize;
    std::size_t m_totalFreeSegmentSize;
    std::size_t m_numAllocationsByType[MEM_ALLOC_COUNT];
};

}   // namespace cookmem

#endif  // COOK_MEM_LOGGER_H
