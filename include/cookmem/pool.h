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
#ifndef COOK_MEM_POOL_H
#define COOK_MEM_POOL_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "config.h"
#include "exception.h"
#include "logger.h"
#include "rbtree.h"
#include "circularlist.h"

namespace cookmem
{

/// @cond INTERNAL
// Bit helpers used by the bin maps (from dlmalloc).  T is unsigned, and
// ~x + 1 is -x without the overflow of negating INT_MIN as an int.
template<class T>
T least_bit(T x)
{
    return x & (~x + 1);
}

template<class T>
T left_bits(T x)
{
    return (x << 1) | (~(x << 1) + 1);
}
/// @endcond

/* Assign the tree bin index for size S to I.  Use the compiler builtins or
   intrinsics for the bit scan if possible. */
#if defined(__GNUC__)
#define cookmem_getLargeBinIndex(S, I)\
{\
  std::size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K = (unsigned) sizeof(unsigned int)*__CHAR_BIT__ - 1 - (unsigned) __builtin_clz((unsigned int) X); \
    I =  (BinIndexType)((K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1)));\
  }\
}

#elif defined (__INTEL_COMPILER)
#define cookmem_getLargeBinIndex(S, I)\
{\
  std::size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K = _bit_scan_reverse (X); \
    I =  (BinIndexType)((K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1)));\
  }\
}

#elif defined(_MSC_VER) && _MSC_VER>=1300
#define cookmem_getLargeBinIndex(S, I)\
{\
  std::size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K;\
    _BitScanReverse((DWORD *) &K, (DWORD) X);\
    I =  (BinIndexType)((K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1)));\
  }\
}

#else /* GNUC */
#define cookmem_getLargeBinIndex(S, I)\
{\
  std::size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int Y = (unsigned int)X;\
    unsigned int N = ((Y - 0x100) >> 16) & 8;\
    unsigned int K = (((Y <<= N) - 0x1000) >> 16) & 4;\
    N += K;\
    N += K = (((Y <<= K) - 0x4000) >> 16) & 2;\
    K = 14 - N + ((Y <<= K) >> 15);\
    I = (K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1));\
  }\
}
#endif /* GNUC */

#if defined(__GNUC__)
#define cookmem_bit2treeIndex(X, I)\
{\
  unsigned int J;\
  J = __builtin_ctz(X); \
  I = (BinIndexType)J;\
}

#elif defined (__INTEL_COMPILER)
#define cookmem_bit2treeIndex(X, I)\
{\
  unsigned int J;\
  J = _bit_scan_forward (X); \
  I = (BinIndexType)J;\
}

#elif defined(_MSC_VER) && _MSC_VER>=1300
#define cookmem_bit2treeIndex(X, I)\
{\
  unsigned int J;\
  _BitScanForward((DWORD *) &J, X);\
  I = (BinIndexType)J;\
}

#elif USE_BUILTIN_FFS
#define cookmem_bit2treeIndex(X, I) I = ffs(X)-1

#else
#define cookmem_bit2treeIndex(X, I)\
{\
  unsigned int Y = X - 1;\
  unsigned int K = Y >> (16-4) & 16;\
  unsigned int N = K;        Y >>= K;\
  N += K = Y >> (8-3) &  8;  Y >>= K;\
  N += K = Y >> (4-2) &  4;  Y >>= K;\
  N += K = Y >> (2-1) &  2;  Y >>= K;\
  N += K = Y >> (1-0) &  1;  Y >>= K;\
  I = (BinIndexType)(N + Y);\
}
#endif /* GNUC */

/**
 * A memory allocator.
 *
 * Note that this class only provides the algorithm necessary to deal with
 * small memory chunks.  The actual ability to get memory segment (such
 * as mmap memory pages) is deferred to memory arena.
 */
template<class Arena, class Logger, class TreeType = PtrRBTree, class T=void>
class MemPool
{
public:
    /** size type */
    typedef std::size_t     size_type;
    /** pointer difference type */
    typedef std::ptrdiff_t  difference_type;
    /** value type */
    typedef T               value_type;
    /** pointer type */
    typedef T*              pointer;
    /** const pointer type */
    typedef const T*        const_pointer;

private:
    typedef unsigned int    BinIndexType;
    typedef TreeType        PtrTreeType;

private:
    /**
     * Basic memory chunk header.
     *
     * m_size is the chunk size, with BIT_USED set if the chunk is in use and
     * BIT_NOTEXACTSIZE set if the exact user size is stored in the last byte
     * of the chunk.  m_prevFootSize is the size of the previous chunk, with
     * BIT_USED set if that chunk is in use.
     */
    class MemChunk
    {
    public:
        /**
         * Chunk sizes are multiples of ALLOCATION_ALIGNMENT, which is at
         * least 8 bytes.  So the lower bits of sizes are available for flags.
         */
        static const size_type  BIT_MASK = ~static_cast<size_type> (ALLOCATION_ALIGNMENT - 1);
        static const size_type  BIT_USED = 1;
        static const size_type  BIT_NOTEXACTSIZE = 2;

    private:
        size_type   m_prevFootSize;     /* Size of previous chunk (if free).  */
        size_type   m_size;            /* Size */

    public:
        inline size_type
        getChunkSize () const
        {
            return m_size & BIT_MASK;
        }

        inline size_type
        getPrevChunkSize () const
        {
            return m_prevFootSize & BIT_MASK;
        }

        inline void
        setFreeChunkSize (size_type chunkSize)
        {
            m_size = chunkSize;
            getNextChunk (chunkSize)->m_prevFootSize = chunkSize;
        }

        inline void
        setFence ()
        {
            m_size = BIT_USED;
        }

        /**
         * Clear the used bit, so that a later double free of this chunk is
         * detected even if the chunk is merged into the free chunk before
         * it, which leaves this header inside the merged chunk.
         */
        inline void
        clearUsed ()
        {
            m_size &= ~BIT_USED;
        }

        inline bool
        isUsed () const
        {
            return m_size & BIT_USED;
        }

        inline bool
        isPrevChunkUsed () const
        {
            return m_prevFootSize & BIT_USED;
        }

        inline size_type
        getUserSize () const
        {
            size_type chunkSize = getChunkSize ();
            if (m_size & BIT_NOTEXACTSIZE)
            {
                char diff = *((char*)(this) + chunkSize - 1);
                if (diff <= 0)
                {
                    throw Exception (MEM_ERROR_PADDING, "Invalid padding byte value", __FILE__, __LINE__);
                }
                return chunkSize - CHUNK_OVERHEAD - diff;
            }
            return chunkSize - CHUNK_OVERHEAD;
        }

        inline void
        setUsedSize (bool storingExactSize, size_type userSize)
        {
            size_type chunkSize = getChunkSize ();
            // Reset the flags, since the chunk may have been in use with a
            // different size (see reallocate ()).
            m_size = chunkSize | BIT_USED;
            if (storingExactSize)
            {
                userSize += CHUNK_OVERHEAD;
                if (userSize != chunkSize)
                {
                    m_size |= BIT_NOTEXACTSIZE;
                    char diff = (char)(chunkSize - userSize);
                    *(((char*)this) + chunkSize - 1) = diff;
                }
            }
            getNextChunk (chunkSize)->m_prevFootSize = chunkSize | BIT_USED;
        }

        inline MemChunk*
        getPrevChunk ()
        {
            return (MemChunk*)((char*)this - getPrevChunkSize ());
        }

        inline MemChunk*
        getNextChunk (size_type chunkSize)
        {
            return (MemChunk*)((char*)this + chunkSize);
        }

        inline const MemChunk*
        getNextChunk () const
        {
            return (const MemChunk*)((char*)this + getChunkSize ());
        }

        bool
        isValidSize ()
        {
            const MemChunk* nextChunk = getNextChunk (getChunkSize ());
            return getChunkSize () == nextChunk->getPrevChunkSize () &&
                   isUsed () == nextChunk->isPrevChunkUsed ();
        }

        void
        print () const
        {
            printf ("-- chunk[ %p ]: size: %zu, used = %s, exact size: %zu\n",
                    static_cast<const void*> (this),
                    getChunkSize (),
                    (isUsed () ? "true " : "false"),
                    getUserSize ());
        }
    };

    /**
     * SmallMemChunk of certain sizes are stored as a DLL.
     */
    struct SmallMemChunk : public MemChunk
    {
        SmallMemChunk* next;       /* double links -- used only if free. */
        SmallMemChunk* prev;
    };

    class MemSegment
    {
        // MemPool takes the offset of m_pad (see SEGMENT_HEADER).
        friend class MemPool;

    private:
        /**
         * Allocated size.
         */
        size_type   m_size;
        /**
         * SLL of segments.
         */
        MemSegment* m_next;
        /**
         * A simple padding of value 0 with used bit set.  This is to make
         * sure the prevFootSize of memory chunk is 0 and stop the free memory
         * coalescing for the current segment.
         */
        size_type   m_pad;

    public:
        /**
         * Initialize a segment and return the initial memory chunk.
         *
         * @param   segSize
         *          the size of the segment
         * @return  the initial memory chunk inside the segment.
         */
        MemChunk*
        init (size_type segSize)
        {
            m_size = segSize;
            m_pad = MemChunk::BIT_USED;

            // We do not initiate m_next since it will be assigned
            // by the caller anyways.

            size_type  chunkSize = getMaxChunkSize ();
            MemChunk* chunk = getFirstChunk ();
            chunk->setFreeChunkSize (chunkSize);
            chunk->getNextChunk (chunkSize)->setFence ();
            return chunk;
        }

        MemChunk*
        getFirstChunk ()
        {
            return (MemChunk*)(((char*)this) + offsetof (MemSegment, m_pad));
        }

        const MemChunk*
        getFirstChunk () const
        {
            return (const MemChunk*)(((char*)this) + offsetof (MemSegment, m_pad));
        }

        MemChunk*
        getPastLastChunk ()
        {
            return (MemChunk*)(((char*)getFirstChunk ()) + getMaxChunkSize ());
        }

        const MemChunk*
        getPastLastChunk () const
        {
            return (MemChunk*)(((char*)getFirstChunk ()) + getMaxChunkSize ());
        }

        size_type
        getSize () const
        {
            return m_size;
        }

        MemSegment*
        getNext () const
        {
            return m_next;
        }

        void
        setNext (MemSegment* seg)
        {
            m_next = seg;
        }

        size_type
        getMaxChunkSize () const
        {
            return (m_size - SEGMENT_OVERHEAD) & MemChunk::BIT_MASK;
        }

        void
        print () const
        {
            printf ("== segment %p, size: %zu, header: %zu, footer: %zu\n", static_cast<const void*> (this), m_size, offsetof(MemSegment, m_pad), sizeof(MemChunk));
            const MemChunk* chunk = getFirstChunk ();
            const MemChunk* lastChunk = getPastLastChunk ();
            for (;;)
            {
                if (chunk < lastChunk)
                {
                    chunk->print ();
                    chunk = chunk->getNextChunk ();
                }
                else
                {
                    printf ("-- chunk[ %p ]: size: %zu\n",
                            static_cast<const void*> (chunk),
                            chunk->getChunkSize ());
                    break;
                }
            }
        }
    };

private:
    /**
     * The alignment of allocations (see ALLOCATION_ALIGNMENT).
     *
     * The result is that all the pointers allocated and corresponding sizes
     * would have the lower bits free.
     */
    static const size_type  ALIGNMENT = ALLOCATION_ALIGNMENT;
    /**
     * The mask used to calculate the alignment.
     */
    static const size_type  ALIGN_MASK = ALIGNMENT - 1;
    // MemChunk stores BIT_USED and BIT_NOTEXACTSIZE in the low bits of chunk
    // sizes, so the alignment must leave at least two low bits free.
    static_assert (ALIGNMENT >= 4, "ALLOCATION_ALIGNMENT must be at least 4.");
    static_assert (ALIGN_MASK == ~MemChunk::BIT_MASK,
                   "ALLOCATION_ALIGNMENT must match MemChunk::BIT_MASK.");
    /**
     * The number of fixed small segment bins.
     *
     * This number is basically fixed due to the values of SMALLBIN_SHIFT
     * and TREEBIN_SHIFT.
     */
    static const size_type  NSMALLBINS = 32;
    /**
     * The number of fixed large segment bins.
     */
    static const size_type  NTREEBINS = 32;
    /**
     * The shift used to calculate the small memory chunk bin.
     */
    static const size_type  SMALLBIN_SHIFT = 3;
    /**
     * The shift used to calculate the large memory chunk bin.
     */
    static const size_type  TREEBIN_SHIFT = 8;
    /**
     * The smallest large memory chunk size.
     */
    static const size_type  MIN_LARGE_SIZE = (1UL << TREEBIN_SHIFT);
    /**
     * The extra bytes needed for a memory chunk.
     */
    static const size_type  CHUNK_OVERHEAD = sizeof(MemChunk);
    /**
     * The smallest request size that would fit in a large memory chunk.
     */
    static const size_type  MIN_LARGE_REQUEST = MIN_LARGE_SIZE - CHUNK_OVERHEAD - ALIGN_MASK;
    /**
     * The smallest memory chunk size.
     */
    static const size_type  MIN_CHUNK_SIZE = ((sizeof(SmallMemChunk) + ALIGN_MASK) & ~ALIGN_MASK);
    /**
     * The largest memory request size allowed.
     */
    static const size_type  MAX_REQUEST = (static_cast<size_type>(-static_cast<long>(MIN_CHUNK_SIZE)) << 2);
    /**
     * The largest small request size that can fit into the smallest
     * small memory chunk.
     */
    static const size_type  MIN_REQUEST = MIN_CHUNK_SIZE - CHUNK_OVERHEAD;
    /**
     * The size of the segment header before MemSegment::m_pad.  It is
     * defined here rather than inside MemSegment, because offsetof cannot be
     * used while the class is still incomplete.
     */
    static const size_type  SEGMENT_HEADER = offsetof (MemSegment, m_pad);
    /**
     * The number of extra bytes needed for a segment.
     */
    static const size_type  SEGMENT_OVERHEAD = ((((SEGMENT_HEADER + ALIGN_MASK) & ~ALIGN_MASK) + sizeof(MemChunk) + +ALIGN_MASK) & ~ALIGN_MASK);
private:
    /**
     * The memory arena used to allocate / free large memory segments.
     */
    Arena&          m_arena;
    /**
     * The logging facility.
     */
    Logger&         m_logger;

    /**
     * The maximum foot print allowed.  0 is no limit.
     */
    size_type       m_footprintLimit;

    /**
     * SLL of memory segments obtained from arena
     */
    MemSegment*     m_segList;
    /**
     * The number of deallocations between empty-segment trimming passes.
     */
    size_type       m_trimEmptySegmentsInterval;
    /**
     * The number of deallocations since the last empty-segment trimming pass.
     */
    size_type       m_numFrees;
    /**
     * Freed allocations at least this large release their segment right away
     * if it becomes entirely free.  0 disables this.
     */
    size_type       m_immediateReleaseThreshold;
    BinIndexType    m_smallMap;
    BinIndexType    m_treeMap;
    /**
     * The designated free chunk reserved for fast allocation.
     */
    MemChunk*       m_dv;
    size_type       m_dvSize;

    /**
     * LinkedLists for small chunks
     */
    CircularList<SmallMemChunk> m_smallLists[NSMALLBINS];
    /**
     * Red-black trees for large chunks
     */
    PtrTreeType     m_largeTrees[NTREEBINS];

    // memory foot print tracking
    /**
     * The current memory foot print.
     */
    size_type       m_footprint;
    /**
     * The largest memory foot print at some point of time.
     */
    size_type       m_maxFootprint;
    /**
     * If we want to store the exact user size.
     */
    bool            m_storingExactSize;
    /**
     * If we want to pad to do strict boundary check, it is quite necessary
     * to pad some bytes at the end of user allocation.  It can be used to
     * detect the common issue of having extra bytes padded at the end.
     */
    const std::uint8_t  m_padding;
    /**
     * The padding byte.
     */
    char            m_paddingByte;
public:
    /**
     * Constructor
     *
     * @param   arena
     *          Memory arena used to allocate memory segments.
     * @param   logger
     *          Logging facility used to log memory allocation and deallocation.
     * @param   paddingSize
     *          The number of bytes to pad after each allocation to detect
     *          out of bound access.  0 disables the padding check.
     */
    MemPool (Arena&         arena,
             Logger&        logger,
             std::uint8_t   paddingSize = DEFAULT_PADDING_SIZE)
    : m_arena (arena),
      m_logger (logger),
      m_footprintLimit (0),
      m_segList (nullptr),
      m_trimEmptySegmentsInterval (DEFAULT_TRIM_EMPTY_SEGMENTS_INTERVAL),
      m_numFrees (0),
      m_immediateReleaseThreshold (DEFAULT_IMMEDIATE_RELEASE_THRESHOLD),
      m_smallMap (0),
      m_treeMap (0),
      m_dv (nullptr),
      m_dvSize (0),
      m_smallLists (),
      m_largeTrees (),
      m_footprint (0),
      m_maxFootprint (0),
      m_storingExactSize (paddingSize != 0),
      m_padding (paddingSize),
      m_paddingByte (DEFAULT_PADDING_BYTE)
    {
        if (m_padding > MAX_PADDING_SIZE)
        {
            throw Exception (MEM_ERROR_GENERAL, "padding size cannot exceed 8 bytes.", __FILE__, __LINE__);
        }
    }

    /**
     * Destructor.
     *
     * It releases all the memory segments used.
     */
    ~MemPool()
    {
        MemSegment* seg = m_segList;
        while (seg)
        {
            void* ptr = seg;
            size_type size = seg->getSize ();
            seg = seg->getNext ();
            m_logger.logFreeSegment (ptr, size);
            m_arena.freeSegment (ptr, size);
        }
    }

    /**
     * Set the maximum memory footprint limit.  Allocations that would need
     * new memory segments beyond this limit fail and return nullptr.
     *
     * @param   footprintLimit
     *          the maximum total size of the memory segments in bytes.
     *          0 means no limit.
     */
    void
    setFootprintLimit (size_type footprintLimit)
    {
        m_footprintLimit = footprintLimit;
    }

    /**
     * Get the memory footprint limit.
     *
     * @return  the memory footprint limit
     */
    size_type
    getFootprintLimit () const
    {
        return m_footprintLimit;
    }

    /**
     * Set the number of deallocations between empty-segment trimming passes.
     *
     * Memory segments that become entirely free are kept for later
     * allocations.  After every interval deallocations, the segments that
     * are entirely free are released to the arena.  A smaller interval keeps
     * the footprint smaller, but releases and obtains segments more often.
     *
     * @param   interval
     *          the number of deallocations between the passes.  The default
     *          is DEFAULT_TRIM_EMPTY_SEGMENTS_INTERVAL.  1 (or 0) releases
     *          empty segments on every deallocation.
     */
    void
    setTrimEmptySegmentsInterval (size_type interval)
    {
        m_trimEmptySegmentsInterval = interval;
    }

    /**
     * Get the number of deallocations between empty-segment trimming passes.
     *
     * @return  the number of deallocations between empty-segment trimming
     *          passes.
     */
    size_type
    getTrimEmptySegmentsInterval () const
    {
        return m_trimEmptySegmentsInterval;
    }

    /**
     * Set the size at which freed allocations release their memory right
     * away.
     *
     * A large allocation usually gets a segment of its own.  When an
     * allocation at least this large is freed and its segment becomes
     * entirely free, the segment is released to the arena immediately,
     * instead of at the next empty-segment trimming pass.  This is similar
     * to the mmap threshold of dlmalloc.
     *
     * @param   threshold
     *          the allocation size in bytes.  The default is
     *          DEFAULT_IMMEDIATE_RELEASE_THRESHOLD.  0 disables the
     *          immediate release.
     */
    void
    setImmediateReleaseThreshold (size_type threshold)
    {
        m_immediateReleaseThreshold = threshold;
    }

    /**
     * Get the size at which freed allocations release their memory right
     * away.
     *
     * @return  the allocation size in bytes.  0 means disabled.
     */
    size_type
    getImmediateReleaseThreshold () const
    {
        return m_immediateReleaseThreshold;
    }

    /**
     * Allocate memory from the memory pool.
     *
     * @param   userSize
     *          memory request size.
     * @return  the memory region that is at least the request size.
     *          nullptr if the request cannot be satisfied.
     */
    T*
    allocate (size_type userSize)
    {
        MemAllocType_et allocType;
        MemChunk* chunk = allocateChunk (getMinAllocSize (userSize), allocType);
        if (chunk == nullptr)
        {
            m_logger.logAllocation (nullptr, userSize, MEM_ALLOC_BIG);
            return nullptr;
        }
        setUsed (chunk, userSize);
        return getUserPointer (chunk, allocType);
    }

    /**
     * Allocate memory with an alignment larger than ALLOCATION_ALIGNMENT.
     *
     * The memory is freed with deallocate () like any other allocation.
     * reallocate () does not keep the alignment if it moves the memory.
     *
     * @param   userSize
     *          memory request size.
     * @param   alignment
     *          the alignment of the returned pointer.  It must be a power of
     *          2.  An alignment up to ALLOCATION_ALIGNMENT is the same as
     *          allocate ().
     * @return  the memory region that is at least the request size and is
     *          aligned to the alignment.  nullptr if the alignment is not a
     *          power of 2, or if the request cannot be satisfied.
     */
    T*
    allocateAligned (size_type userSize, size_type alignment)
    {
        if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        {
            m_logger.logAllocation (nullptr, userSize, MEM_ALLOC_BIG);
            return nullptr;
        }
        if (alignment <= ALIGNMENT)
        {
            return allocate (userSize);
        }

        size_type allocSize = getMinAllocSize (userSize);
        if (allocSize >= MAX_REQUEST || alignment >= MAX_REQUEST - allocSize)
        {
            m_logger.logAllocation (nullptr, userSize, MEM_ALLOC_BIG);
            return nullptr;
        }
        size_type chunkSize = (allocSize < MIN_REQUEST) ? MIN_CHUNK_SIZE : calcChunkSize (allocSize);

        // Allocate enough room to find an aligned position inside the chunk
        // that leaves a free chunk of at least MIN_CHUNK_SIZE in front of it.
        MemAllocType_et allocType;
        MemChunk* chunk = allocateChunk (chunkSize + alignment + MIN_CHUNK_SIZE - CHUNK_OVERHEAD, allocType);
        if (chunk == nullptr)
        {
            m_logger.logAllocation (nullptr, userSize, MEM_ALLOC_BIG);
            return nullptr;
        }

        std::uintptr_t mem = reinterpret_cast<std::uintptr_t> (chunk2Mem (chunk));
        if ((mem & (alignment - 1)) != 0)
        {
            // Free the memory in front of the aligned position.  The chunk
            // before it is in use, since free chunks are always coalesced,
            // so the leading chunk does not need to be coalesced.
            std::uintptr_t alignedMem = (mem + alignment - 1) & ~static_cast<std::uintptr_t> (alignment - 1);
            size_type leadSize = static_cast<size_type> (alignedMem - mem);
            if (leadSize < MIN_CHUNK_SIZE)
            {
                leadSize += alignment;
            }
            MemChunk* alignedChunk = chunk->getNextChunk (leadSize);
            size_type alignedSize = chunk->getChunkSize () - leadSize;

            chunk->setFreeChunkSize (leadSize);
            addChunk (chunk);
            alignedChunk->setFreeChunkSize (alignedSize);
            chunk = alignedChunk;
        }

        // Free the memory after the requested size.
        size_type remainSize = chunk->getChunkSize () - chunkSize;
        if (remainSize >= MIN_CHUNK_SIZE)
        {
            MemChunk* remainChunk = chunk->getNextChunk (chunkSize);
            MemChunk* nextChunk = remainChunk->getNextChunk (remainSize);
            if (!nextChunk->isUsed ())
            {
                removeChunk (nextChunk);
                remainSize += nextChunk->getChunkSize ();
            }
            chunk->setFreeChunkSize (chunkSize);
            remainChunk->setFreeChunkSize (remainSize);
            addChunk (remainChunk);
        }

        setUsed (chunk, userSize);
        return getUserPointer (chunk, allocType);
    }

    /**
     * This function reallocate a memory.
     *
     * The memory is resized in place when possible (see resizeInPlace ()).
     * Otherwise new memory is allocated, the data is copied, and the old
     * memory is freed.
     *
     * @param   ptr
     *          the current memory pointer.
     * @param   newUserSize
     *          the new size
     * @return  the reallocated pointer.  If the new size cannot be
     *          satisfied, a nullptr is returned and the old pointer
     *          remains valid.
     */
    T*
    reallocate (T* ptr, size_type newUserSize)
    {
        if (ptr == nullptr)
        {
            return allocate (newUserSize);
        }

        if (resizeInPlace (ptr, newUserSize))
        {
            return ptr;
        }

        // allocate () fails for a request too large to represent.
        size_type oldUserSize = mem2Chunk (ptr)->getUserSize ();
        T* newPtr = allocate (newUserSize);
        if (newPtr)
        {
            memcpy (newPtr, ptr, oldUserSize);
            deallocate (ptr);
        }
        return newPtr;
    }

    /**
     * Resize memory without moving it, like realloc_in_place in dlmalloc.
     *
     * Shrinking always succeeds, and the unused tail is freed.  Growing
     * succeeds if the memory after the allocation is free and large enough.
     *
     * @param   ptr
     *          the current memory pointer.
     * @param   newUserSize
     *          the new size
     * @return  true if the memory now has the new size at the same address.
     *          false if it could not be resized in place, or if ptr is
     *          nullptr; the memory is then unchanged.
     */
    bool
    resizeInPlace (T* ptr, size_type newUserSize)
    {
        if (ptr == nullptr)
        {
            return false;
        }

        MemChunk* chunk = mem2Chunk (ptr);

        // The new chunk must also hold the padding, and a request too large
        // to represent must not wrap around to a small chunk size.
        size_type newAllocSize = getMinAllocSize (newUserSize);
        if (newAllocSize >= MAX_REQUEST)
        {
            return false;
        }
        size_type newChunkSize = (newAllocSize < MIN_REQUEST) ? MIN_CHUNK_SIZE : calcChunkSize (newAllocSize);

        size_type chunkSize = chunk->getChunkSize ();
        if (chunkSize < newChunkSize)
        {
            // Grow in place if the next chunk is free and large enough.
            MemChunk* nextChunk = chunk->getNextChunk (chunkSize);
            if (nextChunk->isUsed () ||
                chunkSize + nextChunk->getChunkSize () < newChunkSize)
            {
                return false;
            }

            // The size before, as logged; read it before setUsed () changes it.
            size_type oldUserSize = chunk->getUserSize ();
            size_type mergedSize = chunkSize + nextChunk->getChunkSize ();
            removeChunk (nextChunk);

            // The chunk after nextChunk is in use, since free chunks are
            // always coalesced, so the remainder does not need merging.
            size_type remainSize = mergedSize - newChunkSize;
            if (remainSize >= MIN_CHUNK_SIZE)
            {
                chunk->setFreeChunkSize (newChunkSize);
                MemChunk* remainChunk = chunk->getNextChunk (newChunkSize);
                remainChunk->setFreeChunkSize (remainSize);
                addChunk (remainChunk);
            }
            else
            {
                chunk->setFreeChunkSize (mergedSize);
            }
            setUsed (chunk, newUserSize);
            m_logger.logReallocation (ptr, oldUserSize, chunk->getUserSize ());
            return true;
        }

        // Shrink, freeing the unused tail if it is large enough, merged with
        // the next chunk if that one is free.
        size_type oldUserSize = chunk->getUserSize ();
        size_type remainSize = chunkSize - newChunkSize;
        if (remainSize >= MIN_CHUNK_SIZE)
        {
            MemChunk* remainChunk = chunk->getNextChunk (newChunkSize);
            MemChunk* nextChunk = remainChunk->getNextChunk (remainSize);

            if (!nextChunk->isUsed ())
            {
                removeChunk (nextChunk);
                remainSize += nextChunk->getChunkSize ();
            }

            chunk->setFreeChunkSize (newChunkSize);
            setUsed (chunk, newUserSize);
            remainChunk->setFreeChunkSize (remainSize);
            addChunk (remainChunk);
        }
        else
        {
            setUsed (chunk, newUserSize);
        }
        m_logger.logReallocation (ptr, oldUserSize, chunk->getUserSize ());
        return true;
    }

    /**
     * Allocate n items of certain size.  The memory allocated is zeroed.
     *
     * @param   num
     *          number of elements
     * @param   size
     *          element size
     * @return  memory allocated.  nullptr if num * size overflows, or if the
     *          request cannot be satisfied.
     */
    T*
    callocate (size_type num, size_type size)
    {
        if (num != 0 && size > (MAX_REQUEST / num))
        {
            m_logger.logAllocation (nullptr, MAX_REQUEST, MEM_ALLOC_BIG);
            return nullptr;
        }
        size_type totalSize = size * num;
        T* ptr = allocate (totalSize);
        if (ptr)
        {
            memset (ptr, 0, totalSize);
        }
        return ptr;
    }

    /**
     * Free a piece of memory previously allocated by this memory pool.
     *
     * @param   ptr
     *          a piece of memory to be freed.
     * @param   size
     *          ignored.
     */
    void
    deallocate (T* ptr, size_type size = 0)
    {
        if (ptr != nullptr)
        {
            MemChunk* chunk = mem2Chunk (ptr);
            if (!chunk->isUsed())
            {
                throw Exception (MEM_ERROR_DOUBLE_FREE, "potentially freeing an unused pointer", __FILE__, __LINE__);
            }
            COOKMEM_ASSERT (chunk->isValidSize ());
            size_type chunkSize = chunk->getChunkSize ();
            if (m_padding != 0)
            {
                COOKMEM_ASSERT (chunkSize > CHUNK_OVERHEAD);
                size_type userSize = chunk->getUserSize () + CHUNK_OVERHEAD;
                COOKMEM_ASSERT (userSize < chunkSize);
#if COOKMEM_DEBUG > 0
                char diff = (char)(chunkSize - userSize);
                COOKMEM_ASSERT ((diff > 0) && ((userSize + diff) == chunkSize));
                COOKMEM_ASSERT (*(((char*)chunk) + chunkSize - 1) == diff);
#endif  // COOKMEM_DEBUG

                char* paddingPtr = ((char*)chunk) + userSize;
                for (std::uint8_t i = 0; i < m_padding; ++i)
                {
                    if (paddingPtr[i] != m_paddingByte)
                    {
                        throw Exception (MEM_ERROR_PADDING, "padding byte got modified.", __FILE__, __LINE__);
                    }
                }
            }
            m_logger.logDeallocation (ptr, chunk->getUserSize ());
            chunk->clearUsed ();

            bool        prevChunkUsed = chunk->isPrevChunkUsed ();
            MemChunk*   nextChunk = chunk->getNextChunk (chunkSize);
            bool        nextChunkUsed = nextChunk->isUsed ();

            MemChunk*   freeChunk;

            // Fast path: when only the previous chunk is free, it may be able
            // to absorb this chunk without being moved to a different place.
            // This is common when memory is freed in allocation order.
            if (!prevChunkUsed &&
                nextChunkUsed &&
                growFreeChunkInPlace (chunk->getPrevChunk (), chunkSize))
            {
                freeChunk = chunk->getPrevChunk ();
            }
            else
            {
                freeChunk = chunk;
                size_type   freeChunkSize = chunkSize;

                if (!prevChunkUsed)
                {
                    MemChunk* prevChunk = chunk->getPrevChunk ();
                    removeChunk (prevChunk);
                    freeChunk = prevChunk;
                    freeChunkSize += prevChunk->getChunkSize ();
                }

                if (!nextChunkUsed)
                {
                    freeChunkSize += nextChunk->getChunkSize ();
                    removeChunk (nextChunk);
                }

                freeChunk->setFreeChunkSize (freeChunkSize);
                addChunk (freeChunk);
            }

            // A large allocation that had a segment of its own is returned
            // to the arena right away, rather than at the next trimming pass.
            if (m_immediateReleaseThreshold != 0 &&
                chunkSize >= m_immediateReleaseThreshold &&
                isWholeSegment (freeChunk))
            {
                releaseSegment (freeChunk);
            }

            ++m_numFrees;
            if (m_numFrees >= m_trimEmptySegmentsInterval)
            {
                trimEmptySegments ();
            }
        }
        else
        {
            m_logger.logDeallocation (ptr, size);
        }
    }

    /**
     * Check if a pointer is within the address space of segments owned
     * by this MemPool.
     *
     * @param   ptr
     *          memory pointer
     * @param   checkUsed
     *          check the pointer if it is used.
     * @return  whether the memory address is in the space of segments owned
     *          by this MemPool.
     *          If checkUsed is true, the result is true only if the pointer
     *          is also marked as used.
     */
    bool
    contains (T* ptr, bool checkUsed = false)
    {
        MemSegment* seg = m_segList;
        while (seg)
        {
            if (reinterpret_cast<char*>(ptr) >= reinterpret_cast<char*>(seg) &&
                reinterpret_cast<char*>(ptr) <= (reinterpret_cast<char*>(seg) + seg->getSize ()))
            {
                if (checkUsed)
                {
                    MemChunk* chunk = mem2Chunk (ptr);
                    return chunk->isUsed ();
                }
                return true;
            }
            seg = seg->getNext ();
        }
        return false;
    }

    /**
     * Validate that all memory structures are correct.
     */
    void
    validate ()
    {
        COOKMEM_ASSERT ((m_dv == nullptr && m_dvSize == 0) ||
                        (m_dv != nullptr &&
                         !m_dv->isUsed () &&
                         m_dv->getChunkSize () == m_dvSize));

        // A tree bin is marked in the tree map if and only if its tree is
        // not empty.
        for (BinIndexType binIndex = 0; binIndex < NTREEBINS; ++binIndex)
        {
            COOKMEM_ASSERT (isTreeMapMarked (binIndex) == !treeAt (binIndex).isEmpty ());
        }

        MemSegment* seg = m_segList;
        while (seg)
        {
            size_type   segSize = seg->getSize ();
            size_type   remainSize = segSize;

            MemChunk*   firstChunk = seg->getFirstChunk ();
            MemChunk*   chunk = seg->getPastLastChunk ();

            do
            {
                size_type   prevChunkSize = chunk->getPrevChunkSize ();

                if (prevChunkSize == 0)
                {
                    break;
                }

                COOKMEM_ASSERT (prevChunkSize < remainSize);
                remainSize -= prevChunkSize;

                MemChunk*   prevChunk = chunk->getPrevChunk ();
                COOKMEM_ASSERT (prevChunkSize == prevChunk->getChunkSize ());

                if (!chunk->isPrevChunkUsed ())
                {
                    COOKMEM_ASSERT (isValidChunk (prevChunk));
                    // Free chunks are always coalesced.
                    COOKMEM_ASSERT (chunk->isUsed ());
                }

                chunk = prevChunk;
            }
            while (chunk >= firstChunk);

            COOKMEM_ASSERT (remainSize == SEGMENT_OVERHEAD &&
                            chunk == firstChunk);
            // Only used by the assertions, which release builds omit.
            (void) remainSize;

            seg = seg->getNext ();
        }
    }


    /**
     * Gets the allocated size for a pointer.
     *
     * If we are storing the exact size of the pointer, then the result
     * can match the exact size of user's allocation.
     *
     * Otherwise, the result is the aligned size.
     *
     * @param   ptr
     *          A pointer.
     * @return  the allocation size.
     *          if the pointer is nullptr, the result is 0.
     *          if the pointer is not used, the result is 0.
     */
    size_type
    getUserSize (T* ptr)
    {
        if (ptr == nullptr)
        {
            return 0;
        }
        MemChunk* chunk = mem2Chunk (ptr);
        if (chunk->isUsed ())
        {
            return chunk->getUserSize ();
        }
        return 0;
    }

    /**
     * Call a visitor function for every allocation that has not been freed,
     * like inspect_all in dlmalloc.  This can be used to find memory leaks.
     *
     * The allocations are visited segment by segment, in address order
     * within each segment.  The visitor function must not allocate, free or
     * resize memory of this pool.
     *
     * @param   visitor
     *          the visitor function: a function, lambda or function object
     *          called as visitor (ptr, size) for each allocation, where ptr
     *          is the pointer returned to the user and size is the
     *          allocation size, as returned by getUserSize ().
     * @return  the number of allocations visited.
     */
    template<class Visitor>
    size_type
    forEachAllocation (Visitor visitor) const
    {
        size_type count = 0;
        for (const MemSegment* seg = m_segList; seg != nullptr; seg = seg->getNext ())
        {
            const MemChunk* lastChunk = seg->getPastLastChunk ();
            for (const MemChunk* chunk = seg->getFirstChunk ();
                 chunk < lastChunk;
                 chunk = chunk->getNextChunk ())
            {
                if (chunk->isUsed ())
                {
                    visitor (chunk2Mem (const_cast<MemChunk*> (chunk)), chunk->getUserSize ());
                    ++count;
                }
            }
        }
        return count;
    }

    /**
     * Release all the memory segments held by this MemPool.
     */
    void
    releaseAll ()
    {
        MemSegment* seg = m_segList;
        while (seg)
        {
            void* ptr = seg;
            size_type size = seg->getSize ();
            seg = seg->getNext ();
            m_logger.logFreeSegment (ptr, size);
            m_arena.freeSegment (ptr, size);
        }
        m_segList = nullptr;
        m_smallMap = 0;
        m_treeMap = 0;
        m_footprint = 0;
        m_dv = nullptr;
        m_dvSize = 0;
        memset ((void*)m_largeTrees, 0, sizeof(m_largeTrees));
        memset ((void*)m_smallLists, 0, sizeof(m_smallLists));
    }

    /**
     * Get the current memory footprint.
     *
     * @return  the current memory footprint.
     */
    size_type
    getFootprint () const
    {
        return m_footprint;
    }

    /**
     * Get the maximum memory footprint.
     *
     * @return  the maximum memory footprint.
     */
    size_type
    getMaxFootprint () const
    {
        return m_maxFootprint;
    }

    /**
     * Check whether or not we are storing the exact user size in the memory.
     *
     * @return  whether or not we are storing the exact user size in the memory.
     */
    bool
    isStoringExactSize () const
    {
        return m_storingExactSize;
    }

    /**
     * Set whether or not we should be storing the exact user size in memory.
     *
     * It can only be changed if the current padding is false.
     *
     * @param   b
     *          the boolean choice
     */
    void
    setStoringExactSize (bool b)
    {
        if (m_padding == 0)
        {
            m_storingExactSize = b;
        }
    }

    /**
     * Check whether or not we are padding bytes at the end of the allocated
     * memory.
     *
     * @return  whether or not we are padding bytes at the end of the
     *          allocated memory.
     */
    bool
    isPadding () const
    {
        return m_padding != 0;
    }

    /**
     * Get the number of padding bytes checked after each allocation.
     */
    std::uint8_t
    getPaddingSize () const
    {
        return m_padding;
    }

    /**
     * Get the padding byte.
     *
     * @return  The padding byte.
     */
    std::uint8_t
    getPaddingByte () const
    {
        return (std::uint8_t)m_paddingByte;
    }

    /**
     * Set the padding byte.
     *
     * @param   paddingByte
     *          The padding byte.
     */
    void
    setPaddingByte (std::uint8_t paddingByte)
    {
        m_paddingByte = (char)paddingByte;
    }

    /**
     * Get the tree bin that holds free chunks of a given size.  Chunks of
     * 12 MB and more all go in the last bin.
     *
     * @param   chunkSize
     *          the chunk size, which must be at least the smallest large
     *          chunk size (256 bytes).
     * @return  the tree bin index.
     */
    static unsigned int
    getTreeBinIndex (size_type chunkSize)
    {
        BinIndexType binIndex;
        cookmem_getLargeBinIndex (chunkSize, binIndex);
        return binIndex;
    }

    /**
     * Print the memory segments and chunks of the pool to stdout.  This is
     * for debugging.
     */
    void
    print () const
    {
        printf ("**** context: %p ****\n", static_cast<const void*> (this));

        MemSegment* seg = m_segList;
        while (seg)
        {
            seg->print ();
            seg = seg->getNext ();
        }
    }

private:
    /**
     * Release segments that are entirely free.  This is called automatically
     * after every m_trimEmptySegmentsInterval deallocations (see
     * setTrimEmptySegmentsInterval ()).
     */
    void
    trimEmptySegments ()
    {
        m_logger.logTrimEmptySegments ();
        MemSegment* seg = m_segList;
        MemSegment* prevSeg = nullptr;
        while (seg)
        {
            MemSegment* nextSeg = seg->getNext ();

            size_type   segSize = seg->getSize ();
            MemChunk*   firstChunk = seg->getFirstChunk ();
            if (!firstChunk->isUsed () &&
                firstChunk->getChunkSize () == seg->getMaxChunkSize ())
            {
                // The entire segment is free, so we can just release the
                // segment.
                //
                // If users want to keep the segment around for caching, they
                // can always use CacheMemArena.
                removeChunk (firstChunk);

                m_footprint -= segSize;
                m_logger.logFreeSegment (seg, segSize);
                m_arena.freeSegment (seg, segSize);

                if (prevSeg == nullptr)
                {
                    m_segList = nextSeg;
                }
                if (prevSeg != nullptr)
                {
                    prevSeg->setNext (nextSeg);
                }
            }
            else
            {
                prevSeg = seg;
            }
            seg = nextSeg;
        }
        m_numFrees = 0;
    }

    /**
     * Check if a free chunk spans its entire segment.
     *
     * @param   chunk
     *          a free chunk.
     * @return  true if the chunk is the only chunk of its segment.
     */
    static inline bool
    isWholeSegment (MemChunk* chunk)
    {
        // Only the first chunk of a segment has a previous size of 0, and
        // only the fence at the end of a segment has a size of 0.
        return chunk->getPrevChunkSize () == 0 &&
               chunk->getNextChunk (chunk->getChunkSize ())->getChunkSize () == 0;
    }

    /**
     * Release the segment of a free chunk that spans the entire segment.
     *
     * @param   chunk
     *          the free chunk, which is in a bin or is the designated victim.
     */
    void
    releaseSegment (MemChunk* chunk)
    {
        MemSegment* seg = reinterpret_cast<MemSegment*> (reinterpret_cast<char*> (chunk) - SEGMENT_HEADER);
        COOKMEM_ASSERT (seg->getFirstChunk () == chunk &&
                        chunk->getChunkSize () == seg->getMaxChunkSize ());

        MemSegment* prevSeg = nullptr;
        for (MemSegment* s = m_segList; s != seg; s = s->getNext ())
        {
            COOKMEM_ASSERT (s != nullptr);
            prevSeg = s;
        }
        if (prevSeg == nullptr)
        {
            m_segList = seg->getNext ();
        }
        else
        {
            prevSeg->setNext (seg->getNext ());
        }

        removeChunk (chunk);

        size_type segSize = seg->getSize ();
        m_footprint -= segSize;
        m_logger.logFreeSegment (seg, segSize);
        m_arena.freeSegment (seg, segSize);
    }

    static inline BinIndexType
    idx2bit (BinIndexType binIndex)
    {
        return ((BinIndexType) (1) << (binIndex));
    }

    /**
     * Get the starting user memory address this memory chunk has.
     *
     * @param   chunk
     *          memory chunk
     */
    static inline T*
    chunk2Mem (MemChunk* chunk)
    {
        return ((T*)((char*)(chunk) + sizeof(MemChunk)));
    }

    /**
     * Get the memory chunk that contains the user memory address.
     *
     * @param   mem
     *          user memory
     */
    static inline MemChunk*
    mem2Chunk (T* mem)
    {
        return ((MemChunk*)((char*)(mem) - sizeof(MemChunk)));
    }

    /**
     * From the memory request size, calculate the corresponding chunk size
     * that can hold the request.
     *
     * @param   requestSize
     *          memory request size.
     * @return  the corresponding chunk size.
     */
    static inline size_type
    calcChunkSize (size_type requestSize)
    {
        return (((requestSize) + CHUNK_OVERHEAD + ALIGN_MASK) & ~ALIGN_MASK);
    }

    static inline size_type
    getSmallBinSize (BinIndexType binIndex)
    {
        return ((binIndex) << SMALLBIN_SHIFT);
    }

    static inline BinIndexType
    getSmallBinIndex (size_type size)
    {
        return (BinIndexType) ((size) >> SMALLBIN_SHIFT);
    }

    static inline bool
    isSmallChunk (size_type size)
    {
        return ((size) >> SMALLBIN_SHIFT) < NSMALLBINS;
    }

    inline PtrTreeType&
    treeAt (BinIndexType i)
    {
        return m_largeTrees[i];
    }

    /**
     * Get the smallest chunk size that a tree bin holds.
     *
     * @param   binIndex
     *          the tree bin index.
     * @return  the smallest chunk size of the bin.
     */
    static inline size_type
    getTreeBinMinSize (BinIndexType binIndex)
    {
        return (static_cast<size_type> (1) << ((binIndex >> 1) + TREEBIN_SHIFT)) |
               (static_cast<size_type> (binIndex & 1) << ((binIndex >> 1) + TREEBIN_SHIFT - 1));
    }

    /**
     * Take the best fitting chunk from the tree bins and split it.
     *
     * @param   size
     *          the chunk size needed.
     * @param   limit
     *          the chunk taken must be smaller than this size.  This is used
     *          to prefer the designated victim when it fits better.
     * @return  the chunk, or nullptr if no chunk of at least size and smaller
     *          than limit is free.
     */
    inline MemChunk*
    treeMalloc (size_type size, size_type limit)
    {
        MemChunk* chunk = nullptr;
        size_type  actualSize = size;

        BinIndexType binIndex;
        cookmem_getLargeBinIndex (size, binIndex);

        // Visit only the non-empty bins, starting from the bin of the size,
        // using the tree map.  Bins hold increasing sizes, so stop at a bin
        // whose chunks are all at least the limit.
        BinIndexType bits = m_treeMap & ~(idx2bit (binIndex) - 1);
        while (bits != 0)
        {
            cookmem_bit2treeIndex (least_bit (bits), binIndex);
            if (getTreeBinMinSize (binIndex) >= limit)
            {
                break;
            }
            PtrTreeType& tree = treeAt (binIndex);
            COOKMEM_ASSERT (!tree.isEmpty ());
            chunk = reinterpret_cast<MemChunk*>(tree.remove(actualSize, limit));
            if (chunk != nullptr)
            {
                if (tree.isEmpty ())
                {
                    clearTreeMap (binIndex);
                }
                break;
            }
            bits &= bits - 1;
        }

        if (chunk != nullptr)
        {
            return splitChunk (chunk, size);
        }
        return nullptr;
    }

    /**
     * Find or allocate a free chunk that can hold the allocation size.  The
     * chunk is not marked as used.
     *
     * @param   allocSize
     *          the allocation size, including the padding.
     * @param   allocType
     *          set to where the chunk came from.
     * @return  the chunk, or nullptr if the request cannot be satisfied.
     */
    inline     MemChunk*
    allocateChunk (size_type allocSize, MemAllocType_et& allocType)
    {
        size_type  chunkSize;

        if (allocSize < MIN_LARGE_REQUEST)
        {
            BinIndexType binIndex;
            BinIndexType smallBits;
            chunkSize = (allocSize < MIN_REQUEST) ? MIN_CHUNK_SIZE : calcChunkSize (allocSize);
            binIndex = getSmallBinIndex (chunkSize);
            smallBits = m_smallMap >> binIndex;

            /*
             * Check if two smallest chunk lists that can satisfy the request.
             * Note that since the chunk is quite tight, it is not necessary
             * to split the chunk since there will not be enough rooms.
             */
            if ((smallBits & 0x3U) != 0)
            {
                MemChunk* chunk;
                binIndex += ~smallBits & 1; /* Uses next bin if idx empty */

                chunk = removeSmallChunkList (binIndex);
                COOKMEM_ASSERT(chunk != nullptr && chunk->getChunkSize () == getSmallBinSize (binIndex));

                allocType = MEM_ALLOC_SMALL;
                return chunk;
            }
            else
            {
                if (chunkSize > m_dvSize &&
                    smallBits != 0)
                {
                    /* Use chunk in next nonempty smallbin */
                    MemChunk* chunk;
                    BinIndexType i;
                    BinIndexType leftbits = (smallBits << binIndex) & left_bits(idx2bit(binIndex));
                    BinIndexType leastbit = least_bit(leftbits);
                    cookmem_bit2treeIndex(leastbit, i);

                    chunk = removeSmallChunkList (i);
                    COOKMEM_ASSERT(chunk != nullptr && chunk->getChunkSize () == getSmallBinSize (i));

                    allocType = MEM_ALLOC_SMALL;
                    return splitChunk (chunk, chunkSize);
                }
            }
        }
        else if (allocSize >= MAX_REQUEST)
        {
            // The request size is too big.
            return nullptr;
        }
        else
        {
            chunkSize = calcChunkSize (allocSize);
        }

        MemChunk* chunk;

        if (allocSize < MIN_LARGE_REQUEST &&
            m_dvSize >= chunkSize)
        {
            allocType = MEM_ALLOC_DV;
            return takeDesignatedVictim (chunkSize);
        }

        allocType = MEM_ALLOC_NONE;

        /*
         * Check if we have enough space in existing chunks.
         */
        if (m_treeMap != 0)
        {
            // As in dlmalloc, a tree chunk is only used if it fits better
            // than the designated victim.
            chunk = treeMalloc (chunkSize, (m_dvSize >= chunkSize) ? m_dvSize : ~static_cast<size_type> (0));
            if (chunk != nullptr)
            {
                allocType = MEM_ALLOC_TREE;
            }
        }
        else
        {
            chunk = nullptr;
        }

        if (chunk == nullptr &&
            m_dvSize >= chunkSize)
        {
            chunk = takeDesignatedVictim (chunkSize);
            allocType = MEM_ALLOC_DV;
        }

        /*
         * At this point, we have to allocate memory from arena.
         */
        if (chunk == nullptr)
        {
            chunk = arenaAlloc (chunkSize);
            allocType = MEM_ALLOC_ARENA;
        }

        return chunk;
    }


    /**
     * Split a memory chunk into two parts.  The first part is returned
     * and the second part is saved.
     *
     * @param   chunk
     *          the chunk to be splitted.
     * @param   size
     *          the first chunk size
     * @return  the first part chunk.
     */
    inline MemChunk*
    splitChunk (MemChunk* chunk, size_type size)
    {
        // the size of the 2nd half after the split
        size_type remainSize = chunk->getChunkSize () - size;

        if (remainSize >= MIN_CHUNK_SIZE)
        {
            MemChunk* remainChunk = (MemChunk*)((char*)chunk + size);

            chunk->setFreeChunkSize (size);

            remainChunk->setFreeChunkSize (remainSize);

            addChunk (remainChunk);
        }
        return chunk;
    }

    /**
     * Take the designated victim chunk and split it into two parts.
     *
     * The first part is returned and the second part is saved as the
     * new designated victim.
     *
     * @param   size
     *          The first part size.
     * @return  The first part chunk.
     */
    inline MemChunk*
    takeDesignatedVictim (size_type size)
    {
        MemChunk* chunk = m_dv;
        size_type remainSize = m_dvSize - size;

        if (remainSize >= MIN_CHUNK_SIZE)
        {
            MemChunk* remainChunk = (MemChunk*)((char*)chunk + size);
            chunk->setFreeChunkSize (size);
            remainChunk->setFreeChunkSize (remainSize);

            if (!isSmallChunk (remainSize))
            {
                // The remainder is still a large chunk, so it simply becomes
                // the new designated victim.
                m_dv = remainChunk;
                m_dvSize = remainSize;
                return chunk;
            }

            m_dv = nullptr;
            m_dvSize = 0;
            addSmallChunk (remainChunk);
            return chunk;
        }

        m_dv = nullptr;
        m_dvSize = 0;
        return chunk;
    }

    /**
     * Obtain a large memory chunk from memory arena.
     *
     * @param   chunkSize
     *          the desired memory chunk size.
     * @return  the chunk that fit the chunkSize.  The remaining chunk is
     *          saved.
     */
    MemChunk*
    arenaAlloc (size_type chunkSize)
    {
        size_type estSize;             /* allocation size */

        estSize = chunkSize + SEGMENT_OVERHEAD;
        if (estSize <= chunkSize)
        {
            // Technically can never reach here due to the MAX_REQUEST value
            //
            // However it is always good to have a check.
            return nullptr;
        }

        /*
         * Check user specified foot print limit
         */
        if (m_footprintLimit != 0)
        {
            size_type fp = m_footprint + estSize;
            if (fp > m_footprint && fp > m_footprintLimit)
            {
                // Empty segments count toward the footprint until the next
                // trimming pass.  Release them before giving up.
                trimEmptySegments ();
                fp = m_footprint + estSize;
            }
            if (fp <= m_footprint || fp > m_footprintLimit)
            {
                return nullptr;
            }
        }

        /*
         * Request memory segments from arena.
         */
        size_type segSize = estSize;
        MemSegment* seg = (MemSegment*)m_arena.getSegment (segSize);
        m_logger.logGetSegment (seg, segSize);

        /*
         * Initialize the segment obtained.
         */
        if (seg)
        {
            if ((m_footprint += segSize) > m_maxFootprint)
            {
                m_maxFootprint = m_footprint;
            }

            MemChunk* chunk = seg->init (segSize);

            if (m_segList == nullptr)
            {
                m_segList = seg;
                seg->setNext (nullptr);
            }
            else
            {
                seg->setNext (m_segList);
                m_segList = seg;
            }

            return splitChunk (chunk, chunkSize);
        }
        return nullptr;
    }

    inline CircularList<SmallMemChunk>&
    getSmallChunkList (BinIndexType binIndex)
    {
        return m_smallLists[binIndex];
    }

    inline void
    addSmallChunk (MemChunk* chunk)
    {
        BinIndexType I = getSmallBinIndex(chunk->getChunkSize());

        CircularList<SmallMemChunk>& freeList = getSmallChunkList(I);
        freeList.add ((SmallMemChunk*)chunk);
        if (!isSmallMapMarked(I))
            markSmallMap(I);
    }

    inline MemChunk*
    removeSmallChunkList (BinIndexType binIndex)
    {
        CircularList<SmallMemChunk>& freeList = getSmallChunkList (binIndex);
        SmallMemChunk* chunk = freeList.remove();
        if (freeList.isEmpty ())
        {
            clearSmallMap (binIndex);
        }
        return chunk;
    }

    inline void
    removeSmallChunk (MemChunk* chunk)
    {
        BinIndexType binIndex = getSmallBinIndex (chunk->getChunkSize ());

        CircularList<SmallMemChunk>& freeList = getSmallChunkList (binIndex);
        if (freeList.remove ((SmallMemChunk*)chunk))
        {
            clearSmallMap (binIndex);
        }
    }

    inline void
    markSmallMap (BinIndexType binIndex)
    {
        m_smallMap |= idx2bit(binIndex);
    }

    inline void
    clearSmallMap (BinIndexType binIndex)
    {
        m_smallMap &= ~idx2bit(binIndex);
    }

    inline bool
    isSmallMapMarked (BinIndexType binIndex)
    {
        return m_smallMap & idx2bit (binIndex);
    }

    inline void
    markTreeMap (BinIndexType binIndex)
    {
        m_treeMap |= idx2bit (binIndex);
    }

    inline void
    clearTreeMap (BinIndexType binIndex)
    {
        m_treeMap &= ~idx2bit (binIndex);
    }

    inline bool
    isTreeMapMarked (BinIndexType binIndex)
    {
        return m_treeMap & idx2bit (binIndex);
    }

    void
    addLargeChunk (MemChunk* chunk)
    {
        BinIndexType treeBinIndex;
        cookmem_getLargeBinIndex(chunk->getChunkSize(), treeBinIndex);
        PtrTreeType& tree = treeAt (treeBinIndex);
        if (!isTreeMapMarked (treeBinIndex))
        {
            markTreeMap (treeBinIndex);
        }
        tree.add(chunk, chunk->getChunkSize());
    }

    inline void
    removeLargeChunk (MemChunk* chunk)
    {
        BinIndexType treeBinIndex;
        cookmem_getLargeBinIndex (chunk->getChunkSize(), treeBinIndex);
        PtrTreeType& tree = treeAt (treeBinIndex);

        tree.remove (chunk);
        if (tree.isEmpty ())
        {
            clearTreeMap (treeBinIndex);
        }
    }

    inline bool
    isValidChunk (MemChunk* chunk)
    {
        if (chunk == m_dv)
        {
            return true;
        }

        if (isSmallChunk (chunk->getChunkSize ()))
        {
            BinIndexType I = getSmallBinIndex (chunk->getChunkSize ());

            CircularList<SmallMemChunk>& freeList = getSmallChunkList (I);
            return freeList.contains ((SmallMemChunk*)chunk);
        }
        else
        {
            BinIndexType treeBinIndex;
            cookmem_getLargeBinIndex (chunk->getChunkSize (), treeBinIndex);
            PtrTreeType& tree = treeAt (treeBinIndex);
            return tree.contains (chunk);
        }
    }

    /**
     * Try to grow a free chunk in place, so that it absorbs the memory that
     * immediately follows it.  This avoids removing the chunk from its bin
     * and adding it back.
     *
     * This is done only when the chunk can stay where it is: it is the
     * designated victim, or it is a large chunk that stays in the same tree
     * bin and the same position within the tree.
     *
     * @param   chunk
     *          a free chunk.
     * @param   extraSize
     *          the size of the memory immediately following the chunk.
     * @return  true if the chunk was grown.  false if nothing was changed.
     */
    inline bool
    growFreeChunkInPlace (MemChunk* chunk, size_type extraSize)
    {
        size_type oldSize = chunk->getChunkSize ();
        size_type newSize = oldSize + extraSize;

        if (chunk == m_dv)
        {
            chunk->setFreeChunkSize (newSize);
            m_dvSize = newSize;
            return true;
        }

        if (isSmallChunk (oldSize))
        {
            return false;
        }

        // The tree bin is determined by the two most significant bits of the
        // size (see cookmem_getLargeBinIndex).  If the bits that differ are
        // all below oldSize / 4, the two most significant bits are unchanged,
        // so the chunk stays in the same bin.  This check is conservative.
        if ((oldSize ^ newSize) > (oldSize >> 2))
        {
            return false;
        }

        BinIndexType treeBinIndex;
        cookmem_getLargeBinIndex (oldSize, treeBinIndex);
        if (!treeAt (treeBinIndex).canGrowInPlace (chunk, newSize))
        {
            return false;
        }

        // The size field of the chunk is also the value of its tree node, so
        // this updates the tree as well.
        chunk->setFreeChunkSize (newSize);
        return true;
    }

    inline void
    addChunk (MemChunk* chunk)
    {
        if (isSmallChunk(chunk->getChunkSize ()))
        {
            addSmallChunk (chunk);
        }
        else if (m_dv == nullptr)
        {
            m_dv = chunk;
            m_dvSize = chunk->getChunkSize ();
        }
        else
        {
            addLargeChunk (chunk);
        }
    }

    inline void
    removeChunk (MemChunk* chunk)
    {
        if (chunk == m_dv)
        {
            m_dv = nullptr;
            m_dvSize = 0;
        }
        else if (isSmallChunk (chunk->getChunkSize ()))
        {
            removeSmallChunk (chunk);
        }
        else
        {
            removeLargeChunk (chunk);
        }
    }

    /**
     * Add the configured padding bytes and one byte for the size metadata.
     */
    inline size_t
    getMinAllocSize (size_type userSize)
    {
        if (m_padding == 0)
        {
            return userSize;
        }
        if (userSize > (MAX_REQUEST - m_padding - 1))
        {
            return MAX_REQUEST;
        }
        // +1 for the size metadata
        return userSize + m_padding + 1;
    }

    inline void
    setUsed (MemChunk* chunk, size_type userSize)
    {
        chunk->setUsedSize (m_storingExactSize, userSize);

        // if we are padding, userSize will always be smaller
        // than the chunkSize
        if (m_padding != 0)
        {
            char*       paddingPtr = ((char*)chunk) + userSize + CHUNK_OVERHEAD;
            for (std::uint8_t i = 0; i < m_padding; ++i)
            {
                paddingPtr[i] = m_paddingByte;
            }
        }
    }

    inline T*
    getUserPointer (MemChunk* chunk, MemAllocType_et allocType)
    {
        T* userPtr = chunk2Mem (chunk);
        // Log the size the chunk reports, which deallocate () logs too.
        // Without the exact size, it is the requested size rounded up.
        m_logger.logAllocation (userPtr, chunk->getUserSize (), allocType);
        return userPtr;
    }
};

#undef cookmem_getLargeBinIndex
#undef cookmem_bit2treeIndex

}   // namespace cookmem

#endif  // COOK_MEM_POOL_H
