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
#include <cstdio>

#include "exception.h"

namespace cookmem
{

/**
 * How an allocation was satisfied.  StatsLogger counts allocations by type.
 */
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
public:
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
     *          the segment released.
     * @param   segmentSize
     *          the segment size.
     */
    virtual void
    logFreeSegment (void* segment, std::size_t segmentSize) = 0;

    /** Log a pool empty-segment trimming pass. */
    virtual void
    logTrimEmptySegments () = 0;

    /**
     * Log the memory allocation.
     *
     * @param   userPtr
     *          the user pointer allocated, or nullptr if the allocation
     *          failed.
     * @param   userSize
     *          the size of the allocation, as getSize () reports it: the
     *          requested size if the exact size is stored, and otherwise the
     *          requested size rounded up to the alignment.  If the
     *          allocation failed, the requested size.
     * @param   allocType
     *          the type of the allocation.
     */
    virtual void
    logAllocation (void* userPtr, std::size_t userSize, MemAllocType_et allocType) = 0;

    /**
     * Log a reallocation in place, which grows or shrinks an allocation
     * without moving it.  A reallocation that moves the memory is logged as
     * an allocation and a deallocation instead.
     *
     * @param   userPtr
     *          the user pointer, which does not change.
     * @param   oldUserSize
     *          the size before, as getSize () reported it.
     * @param   newUserSize
     *          the size after, as getSize () reports it.
     */
    virtual void
    logReallocation (void* userPtr, std::size_t oldUserSize, std::size_t newUserSize) = 0;

    /**
     * Log the memory deallocation.
     *
     * @param   userPtr
     *          the user pointer allocated.
     * @param   userSize
     *          the size of the allocation, as getSize () reports it, which
     *          is the size its allocation was logged with.
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
     * @param   file
     *          the source file where the error was detected.
     * @param   line
     *          the line number where the error was detected.
     */
    virtual void
    logError (void* userPtr, MemError_et error, const char* file = __FILE__, int line = __LINE__) = 0;
};

/**
 * A very simple MemLogger that does mostly nothing.
 *
 * This class implements the interface of MemLogger without explicitly
 * inheriting from it.  This is to avoid the overhead of virtual function
 * calls and allows inlining of the functions.
 */
class NoActionMemLogger
{
public:
    /** Constructor. */
    NoActionMemLogger ()
    {
    }

    /** Does nothing.  See MemLogger::logGetSegment(). */
    inline void logGetSegment (void*, std::size_t) { }

    /** Does nothing.  See MemLogger::logFreeSegment(). */
    inline void logFreeSegment (void*, std::size_t) { }

    /** Does nothing.  See MemLogger::logTrimEmptySegments(). */
    inline void logTrimEmptySegments () { }

    /** Does nothing.  See MemLogger::logAllocation(). */
    inline void logAllocation (void*, std::size_t, MemAllocType_et) { }

    /** Does nothing.  See MemLogger::logReallocation(). */
    inline void logReallocation (void*, std::size_t, std::size_t) { }

    /** Does nothing.  See MemLogger::logDeallocation(). */
    inline void logDeallocation (void*, std::size_t) { }

    /**
     * Report a memory error by throwing an exception.
     *
     * @param   error
     *          the type of the error encountered.
     * @param   file
     *          the source file where the error was detected.
     * @param   line
     *          the line number where the error was detected.
     * @throws  Exception
     *          always.
     */
    inline void
    logError (void*,
              MemError_et error,
              const char* file = __FILE__,
              int line = __LINE__)
    {
        throw Exception (error, "memory corruption detected.", file, line);
    }
};

/**
 * Memory allocation and segment usage statistics.
 */
struct MemStatistics
{
public:
    MemStatistics ()
    : m_numAllocations (0),
      m_numDeallocations (0),
      m_numGetSegments (0),
      m_numFreeSegments (0),
      m_numTrimEmptySegments (0),
      m_totalAllocated (0),
      m_totalDeallocated (0),
      m_totalGetSegmentSize (0),
      m_totalFreeSegmentSize (0),
      m_currentAllocated (0),
      m_peakAllocated (0),
      m_currentSegments (0),
      m_peakSegments (0),
      m_currentSegmentSize (0),
      m_peakSegmentSize (0),
      m_numAllocationsByType {}
    {
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
    /** Get the number of pool empty-segment trimming passes. */
    std::size_t getNumTrimEmptySegments () const { return m_numTrimEmptySegments; }
    /** Get the total number of bytes allocated. */
    std::size_t getTotalAllocated () const { return m_totalAllocated; }
    /** Get the total number of bytes deallocated. */
    std::size_t getTotalDeallocated () const { return m_totalDeallocated; }
    /** Get the total size of acquired memory segments. */
    std::size_t getTotalGetSegmentSize () const { return m_totalGetSegmentSize; }
    /** Get the total size of released memory segments. */
    std::size_t getTotalFreeSegmentSize () const { return m_totalFreeSegmentSize; }
    /** Get the current number of allocated user bytes. */
    std::size_t getCurrentAllocated () const { return m_currentAllocated; }
    /** Get the peak number of allocated user bytes. */
    std::size_t getPeakAllocated () const { return m_peakAllocated; }
    /** Get the current number of active memory segments. */
    std::size_t getCurrentSegments () const { return m_currentSegments; }
    /** Get the peak number of active memory segments. */
    std::size_t getPeakSegments () const { return m_peakSegments; }
    /** Get the current size of active memory segments. */
    std::size_t getCurrentSegmentSize () const { return m_currentSegmentSize; }
    /** Get the peak size of active memory segments. */
    std::size_t getPeakSegmentSize () const { return m_peakSegmentSize; }

    /// @cond INTERNAL
    // Internal use only.  These setters are used by StatsLogger.
    void setNumAllocations (std::size_t value) { m_numAllocations = value; }
    void setNumAllocations (MemAllocType_et allocType, std::size_t value)
    {
        m_numAllocationsByType[allocType] = value;
    }
    void setNumDeallocations (std::size_t value) { m_numDeallocations = value; }
    void setNumGetSegments (std::size_t value) { m_numGetSegments = value; }
    void setNumFreeSegments (std::size_t value) { m_numFreeSegments = value; }
    void setNumTrimEmptySegments (std::size_t value) { m_numTrimEmptySegments = value; }
    void setTotalAllocated (std::size_t value) { m_totalAllocated = value; }
    void setTotalDeallocated (std::size_t value) { m_totalDeallocated = value; }
    void setTotalGetSegmentSize (std::size_t value) { m_totalGetSegmentSize = value; }
    void setTotalFreeSegmentSize (std::size_t value) { m_totalFreeSegmentSize = value; }
    void setCurrentAllocated (std::size_t value) { m_currentAllocated = value; }
    void setPeakAllocated (std::size_t value) { m_peakAllocated = value; }
    void setCurrentSegments (std::size_t value) { m_currentSegments = value; }
    void setPeakSegments (std::size_t value) { m_peakSegments = value; }
    void setCurrentSegmentSize (std::size_t value) { m_currentSegmentSize = value; }
    void setPeakSegmentSize (std::size_t value) { m_peakSegmentSize = value; }
    /// @endcond

    /**
     * Print the statistics to stdout.
     */
    void
    printStats () const
    {
        std::printf ("current allocations: %zu bytes, peak allocations: %zu bytes, "
                     "current segments: %zu (%zu bytes), peak segments: %zu (%zu bytes)\n",
                     m_currentAllocated,
                     m_peakAllocated,
                     m_currentSegments,
                     m_currentSegmentSize,
                     m_peakSegments,
                     m_peakSegmentSize);
        std::printf ("empty-segment trimming passes: %zu\n", m_numTrimEmptySegments);

        std::printf ("allocations by type:\n");
        for (std::size_t i = 0; i < MEM_ALLOC_COUNT; ++i)
        {
            std::printf ("  %s: %zu\n",
                         getAllocationTypeName (static_cast<MemAllocType_et> (i)),
                         m_numAllocationsByType[i]);
        }
    }

    /**
     * Get a short name for an allocation type, as printed by printStats ().
     *
     * @param   allocType
     *          the allocation type.
     * @return  the name of the allocation type, or "unknown".
     */
    static const char*
    getAllocationTypeName (MemAllocType_et allocType)
    {
        static const char* const names[MEM_ALLOC_COUNT] =
        {
            "none",
            "small",
            "tree",
            "dv",
            "arena",
            "big"
        };
        std::size_t index = static_cast<std::size_t> (allocType);
        return index < MEM_ALLOC_COUNT ? names[index] : "unknown";
    }

private:
    std::size_t m_numAllocations;
    std::size_t m_numDeallocations;
    std::size_t m_numGetSegments;
    std::size_t m_numFreeSegments;
    std::size_t m_numTrimEmptySegments;
    std::size_t m_totalAllocated;
    std::size_t m_totalDeallocated;
    std::size_t m_totalGetSegmentSize;
    std::size_t m_totalFreeSegmentSize;
    std::size_t m_currentAllocated;
    std::size_t m_peakAllocated;
    std::size_t m_currentSegments;
    std::size_t m_peakSegments;
    std::size_t m_currentSegmentSize;
    std::size_t m_peakSegmentSize;
    std::size_t m_numAllocationsByType[MEM_ALLOC_COUNT];
};

/**
 * A MemLogger that tracks allocation and segment usage statistics.
 *
 * This class implements the interface of MemLogger without explicitly
 * inheriting from it.  This is to avoid the overhead of virtual function
 * calls and allows inlining of the functions.
 */
class StatsLogger
{
public:
    /** @copydoc MemLogger::logTrimEmptySegments */
    void
    logTrimEmptySegments ()
    {
        m_stats.setNumTrimEmptySegments (m_stats.getNumTrimEmptySegments () + 1);
    }

    /** @copydoc MemLogger::logGetSegment */
    void
    logGetSegment (void* segment, std::size_t segmentSize)
    {
        if (segment == nullptr)
        {
            return;
        }
        m_stats.setNumGetSegments (m_stats.getNumGetSegments () + 1);
        m_stats.setTotalGetSegmentSize (m_stats.getTotalGetSegmentSize () + segmentSize);

        std::size_t numSegments = m_stats.getCurrentSegments () + 1;
        std::size_t segmentSizeTotal = m_stats.getCurrentSegmentSize () + segmentSize;
        m_stats.setCurrentSegments (numSegments);
        m_stats.setCurrentSegmentSize (segmentSizeTotal);
        if (m_stats.getPeakSegments () < numSegments)
        {
            m_stats.setPeakSegments (numSegments);
        }
        if (m_stats.getPeakSegmentSize () < segmentSizeTotal)
        {
            m_stats.setPeakSegmentSize (segmentSizeTotal);
        }
    }

    /** @copydoc MemLogger::logFreeSegment */
    void
    logFreeSegment (void* segment, std::size_t segmentSize)
    {
        if (segment == nullptr)
        {
            return;
        }
        m_stats.setNumFreeSegments (m_stats.getNumFreeSegments () + 1);
        m_stats.setTotalFreeSegmentSize (m_stats.getTotalFreeSegmentSize () + segmentSize);
        m_stats.setCurrentSegments (m_stats.getCurrentSegments () - 1);
        m_stats.setCurrentSegmentSize (m_stats.getCurrentSegmentSize () - segmentSize);
    }

    /** @copydoc MemLogger::logAllocation */
    void
    logAllocation (void* userPtr, std::size_t userSize, MemAllocType_et allocType)
    {
        if (userPtr != nullptr)
        {
            m_stats.setNumAllocations (m_stats.getNumAllocations () + 1);
            m_stats.setTotalAllocated (m_stats.getTotalAllocated () + userSize);
            std::size_t currentAllocated = m_stats.getCurrentAllocated () + userSize;
            m_stats.setCurrentAllocated (currentAllocated);
            if (m_stats.getPeakAllocated () < currentAllocated)
            {
                m_stats.setPeakAllocated (currentAllocated);
            }
            m_stats.setNumAllocations (allocType,
                                       m_stats.getNumAllocations (allocType) + 1);
        }
    }

    /** @copydoc MemLogger::logReallocation */
    void
    logReallocation (void* userPtr, std::size_t oldUserSize, std::size_t newUserSize)
    {
        if (userPtr != nullptr && newUserSize > oldUserSize)
        {
            std::size_t delta = newUserSize - oldUserSize;
            m_stats.setTotalAllocated (m_stats.getTotalAllocated () + delta);
            std::size_t currentAllocated = m_stats.getCurrentAllocated () + delta;
            m_stats.setCurrentAllocated (currentAllocated);
            if (m_stats.getPeakAllocated () < currentAllocated)
            {
                m_stats.setPeakAllocated (currentAllocated);
            }
        }
        else if (userPtr != nullptr)
        {
            std::size_t delta = oldUserSize - newUserSize;
            m_stats.setTotalDeallocated (m_stats.getTotalDeallocated () + delta);
            m_stats.setCurrentAllocated (m_stats.getCurrentAllocated () - delta);
        }
    }

    /** @copydoc MemLogger::logDeallocation */
    void
    logDeallocation (void* userPtr, std::size_t userSize)
    {
        if (userPtr != nullptr)
        {
            m_stats.setNumDeallocations (m_stats.getNumDeallocations () + 1);
            m_stats.setTotalDeallocated (m_stats.getTotalDeallocated () + userSize);
            m_stats.setCurrentAllocated (m_stats.getCurrentAllocated () - userSize);
        }
    }

    /** @copydoc NoActionMemLogger::logError */
    void
    logError (void*,
              MemError_et error,
              const char* file = __FILE__,
              int line = __LINE__)
    {
        throw Exception (error, "memory corruption detected.", file, line);
    }

    /**
     * Get the statistics.
     *
     * @return  the statistics gathered so far.
     */
    MemStatistics& getStats () { return m_stats; }

    /**
     * Get the statistics.
     *
     * @return  the statistics gathered so far.
     */
    const MemStatistics& getStats () const { return m_stats; }

private:
    MemStatistics m_stats;
};

}   // namespace cookmem

#endif  // COOK_MEM_LOGGER_H
