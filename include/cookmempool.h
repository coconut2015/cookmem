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
#ifndef COOK_MEM_POOL_H
#define COOK_MEM_POOL_H

#include <cstddef>
#include <cstring>

#ifdef WIN32
#include <windows.h>
#endif /* WIN32 */

#include "cookexception.h"
#include "cookptravltree.h"
#include "cookptrcircularlist.h"

namespace cookmem
{

template<class T>
T least_bit(T x)
{
    return ((x) & -(static_cast<int>(x)));
}

template<class T>
T left_bits(T x)
{
    return ((x << 1) | -static_cast<int>(x << 1));
}

/* assign tree index for size S to variable I. Use x86 asm if possible  */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define cookmem_getLargeBinIndex(S, I)\
{\
  unsigned int X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K = (unsigned) sizeof(X)*__CHAR_BIT__ - 1 - (unsigned) __builtin_clz(X); \
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

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
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
template<class Arena, class Logger, class T=void>
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

private:
    /**
     * Basic memory chunk header.
     *
     * For unused memory chunks, the size information is the chunk size.
     * For memory chunks being used, the size information is the request size
     * (without being aligned) and with sign bit being set.
     *
     * The prevFootSize is treated the same manner.
     */
    class MemChunk
    {
    public:
        /**
         * We are using 16-byte alignment.  So the lower 4 bits of pointers
         * and sizes are available to use.
         */
        static const size_type  BIT_MASK = ((size_t)-1) ^ 0x0f;
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

        inline void
        setFreeChunkSize (size_type chunkSize)
        {
            m_size = chunkSize;
            ((MemChunk*)((char*)(this) + (chunkSize)))->m_prevFootSize = chunkSize;
        }

        inline bool
        isUsed () const
        {
            return m_size & BIT_USED;
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
                    throw Exception (MEM_ERROR_PADDING, "Invalid padding byte value");
                }
                return chunkSize - CHUNK_OVERHEAD - diff;
            }
            return chunkSize - CHUNK_OVERHEAD;
        }

        inline void
        setUsedSize (bool storingExactSize, size_type userSize)
        {
            size_type chunkSize = getChunkSize ();
            m_size |= BIT_USED;
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
            ((MemChunk*)((char*) (this) + (chunkSize)))->m_prevFootSize = chunkSize | BIT_USED;
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

            size_type  chunkSize = segSize - SEGMENT_OVERHEAD;
            MemChunk* chunk = (MemChunk*)(((char*)this) + offsetof (MemSegment, m_pad));
            chunk->setFreeChunkSize (chunkSize);
            return chunk;
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
    };

private:
    /**
     * Use 16 byte alignment.
     *
     * The result is that all the pointers allocated and corresponding sizes
     * would have the lower 4 bits free.
     */
    static const size_type  ALIGNMENT = sizeof(size_type) * 2;
    /**
     * The mask used to calculate the alignment.
     */
    static const size_type  ALIGN_MASK = ALIGNMENT - 1;
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
     * The number of extra bytes needed for a segment.
     */
    static const size_type  SEGMENT_OVERHEAD = ((sizeof(MemSegment) - sizeof(size_type) + ALIGN_MASK) & (~ALIGN_MASK)) + sizeof(size_type);
    /**
     * The number of frees before attempting to coalesce memory chunks.
     */
    static const size_type  MAX_RELEASE_CHECK_RATE = 4095;
    /**
     * Default padding bytes for strict bounding check.
     */
    static const char DEFAULT_PADDING_BYTE = (char)0xcd;

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

    size_type       m_release_checks;

    BinIndexType    m_smallMap;
    BinIndexType    m_treeMap;

    /**
     * LinkedLists for small chunks
     */
    CircularList<SmallMemChunk> m_smallLists[NSMALLBINS];
    /**
     * AVL trees for large chunks
     */
    PtrAVLTree      m_largeTrees[NTREEBINS];

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
    const bool      m_padding;
    /**
     * The padding byte.
     */
    char            m_paddingByte;
public:
    /**
     * Constructor
     * @param   arena
     *          memory arena used to allocate memory segments.
     * @param   padding
     *          Should memory pool pad extra bytes after allocated memory
     *          to detect out of bound access.  Depending on the allocated
     *          size, the minimum number of bytes to pad is 1, and the
     *          maximum is 8.
     */
    MemPool (Arena&     arena,
             Logger&    logger,
             bool       padding = false)
    : m_arena (arena),
      m_logger (logger),
      m_footprintLimit (0),
      m_segList (nullptr),
      m_release_checks (0),
      m_smallMap (0),
      m_treeMap (0),
      m_smallLists (),
      m_largeTrees (),
      m_footprint (0),
      m_maxFootprint (0),
      m_storingExactSize (padding),
      m_padding (padding),
      m_paddingByte (DEFAULT_PADDING_BYTE)
    {
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
     * Set the maximum memory footprint limit.
     *
     * @param   footprintLimit
     *
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
    getFootPrintLimit () const
    {
        return m_footprintLimit;
    }

    /**
     * Allocate memory from the memory pool.
     *
     * @param   size
     *          memory request size.
     * @return  the memory region that is at least the request size.
     *          nullptr if the request cannot be satisfied.
     */
    T*
    allocate (size_type userSize)
    {
        size_type  chunkSize;

        size_type allocSize = getMinAllocSize (userSize);

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
                COOKMEM_ASSERT(chunk != nullptr && chunk->getChunkSize() == getSmallBinSize (binIndex));

                setUsed (chunk, userSize);

                return getUserPointer (chunk, userSize);
            }
            else
            {
                if (smallBits != 0)
                {
                    /* Use chunk in next nonempty smallbin */
                    MemChunk* chunk;
                    BinIndexType i;
                    BinIndexType leftbits = (smallBits << binIndex) & left_bits(idx2bit(binIndex));
                    BinIndexType leastbit = least_bit(leftbits);
                    cookmem_bit2treeIndex(leastbit, i);

                    chunk = removeSmallChunkList(i);
                    COOKMEM_ASSERT(chunk != nullptr && chunk->getChunkSize() == getSmallBinSize(i));

                    chunk = splitChunk (chunk, chunkSize);

                    setUsed (chunk, userSize);

                    return getUserPointer (chunk, userSize);
                }
            }
        }
        else if (allocSize >= MAX_REQUEST)
        {
            // The request size is too big.
            m_logger.logAllocation (nullptr, userSize);
            return nullptr;
        }
        else
        {
            chunkSize = calcChunkSize (allocSize);
        }

        MemChunk* chunk;

        /*
         * Check if we have enough space in existing chunks.
         */
        if (m_treeMap != 0)
        {
            chunk = treeMalloc (chunkSize);
        }
        else
        {
            chunk = nullptr;
        }

        /*
         * At this point, we have to allocate memory from arena.
         */
        if (chunk == nullptr)
        {
            chunk = arenaAlloc (chunkSize);

            if (chunk == nullptr)
            {
                m_logger.logAllocation (nullptr, userSize);
                return nullptr;
            }
        }

        setUsed (chunk, userSize);

        return getUserPointer (chunk, userSize);
    }

    /**
     * This function reallocate a memory.
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
            return allocate(newUserSize);
        }

        MemChunk* chunk = mem2Chunk (ptr);
        size_type oldAllocSize = chunk->getUserSize ();
        size_type oldChunkSize = calcChunkSize(oldAllocSize);

        size_type newAllocSize = newUserSize;
        size_type newChunkSize = (newAllocSize < MIN_REQUEST) ? MIN_CHUNK_SIZE : calcChunkSize(newAllocSize);

        if (oldChunkSize < newChunkSize)
        {
            // For now we use a simple algorithm that allocate new memory.
            T* newPtr = allocate (newUserSize);
            if (newPtr)
            {
                memcpy(newPtr, ptr, oldAllocSize);
                deallocate (ptr);
            }
            return newPtr;
        }
        else
        {
            m_logger.logReallocation(ptr, oldAllocSize, newUserSize);

            // simply split the chunk
            splitChunk (chunk, newChunkSize);
            setUsed (chunk, newUserSize);
            return ptr;
        }
    }

    /**
     * Allocate n items of certain size.  The memory allocated is zeroed.
     *
     * @param   num
     *          number of elements
     * @param   size
     *          element size
     * @return  memory allocated.
     */
    T*
    callocate (size_type num, size_type size)
    {
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
                throw Exception (MEM_ERROR_DOUBLE_FREE, "potentially freeing an unused pointer");
            }
            size_type chunkSize = chunk->getChunkSize ();
            if (m_padding)
            {
                COOKMEM_ASSERT (chunkSize > CHUNK_OVERHEAD);
                size_type userSize = chunk->getUserSize () + CHUNK_OVERHEAD;
                COOKMEM_ASSERT (userSize < chunkSize);
                char diff = (char)(chunkSize - userSize);
                COOKMEM_ASSERT ((diff > 0) && ((userSize + diff) == chunkSize));
                COOKMEM_ASSERT (*(((char*)chunk) + chunkSize - 1) == diff);

                char* paddingPtr = ((char*)chunk) + userSize;
                --diff;
                if (diff > 8)
                {
                    diff = 8;
                }
                for (int i = 0; i < diff; ++i)
                {
                    if (paddingPtr[i] != m_paddingByte)
                    {
                        throw Exception (MEM_ERROR_PADDING, "padding byte got modified.");
                    }
                }
            }
            m_logger.logDeallocation (ptr, chunk->getUserSize ());

            chunk->setFreeChunkSize (chunkSize);
            addChunk (chunk);
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
        memset (m_largeTrees, 0, sizeof(m_largeTrees));
        memset (m_smallLists, 0, sizeof(m_smallLists));
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
        if (!m_padding)
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

private:
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

    inline PtrAVLTree&
    treeAt (BinIndexType i)
    {
        return m_largeTrees[i];
    }

    inline MemChunk*
    treeMalloc (size_type size)
    {
        MemChunk* chunk = nullptr;
        size_type  actualSize = size;

        BinIndexType binIndex;
        cookmem_getLargeBinIndex (size, binIndex);

        for (; binIndex < NTREEBINS; ++binIndex)
        {
            PtrAVLTree& tree = treeAt(binIndex);
            if (!tree.isEmpty ())
            {
                chunk = reinterpret_cast<MemChunk*>(tree.remove(actualSize));
                if (chunk != nullptr)
                    break;
            }
        }

        if (chunk != nullptr)
        {
            return splitChunk (chunk, size);
        }
        return nullptr;
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

                m_release_checks = MAX_RELEASE_CHECK_RATE;
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
        PtrAVLTree& tree = treeAt(treeBinIndex);
        if (!isTreeMapMarked(treeBinIndex))
        {
            markTreeMap(treeBinIndex);
        }
        tree.add(chunk, chunk->getChunkSize());
    }

    inline void
    removeLargeChunk (MemChunk* chunk)
    {
        BinIndexType treeBinIndex;
        cookmem_getLargeBinIndex(chunk->getChunkSize(), treeBinIndex);
        PtrAVLTree& tree = treeAt(treeBinIndex);

        tree.remove(chunk);
        if (tree.isEmpty())
        {
            clearTreeMap(treeBinIndex);
        }
    }

    inline void
    addChunk (MemChunk* chunk)
    {
        if (isSmallChunk(chunk->getChunkSize()))
        {
            addSmallChunk (chunk);
        }
        else
        {
            addLargeChunk (chunk);
        }
    }

    inline void
    removeChunk (MemChunk* chunk)
    {
        if (isSmallChunk(chunk->getChunkSize()))
            removeSmallChunk (chunk);
        else
        {
            removeLargeChunk (chunk);
        }
    }

    /**
     * If we are padding bytes, then we need to pad at least 1 byte,
     */
    inline size_t
    getMinAllocSize (size_type userSize)
    {
        return m_padding ? userSize + 1 : userSize;
    }

    inline void
    setUsed (MemChunk* chunk, size_type userSize)
    {
        chunk->setUsedSize (m_storingExactSize, userSize);

        // if we are padding, userSize will always be smaller
        // than the chunkSize
        if (m_padding)
        {
            size_type   chunkSize = chunk->getChunkSize ();
            char*       paddingPtr = ((char*)chunk) + userSize + CHUNK_OVERHEAD;
            char        diff = (char)(chunkSize - CHUNK_OVERHEAD - userSize - 1);
            if (diff > 8)
            {
                diff = 8;
            }
            for (int i = 0; i < diff; ++i)
            {
                paddingPtr[i] = m_paddingByte;
            }
        }
    }

    inline T*
    getUserPointer (MemChunk* chunk, size_type userSize)
    {
        T* userPtr = chunk2Mem (chunk);
        m_logger.logAllocation (userPtr, userSize);
        return userPtr;
    }
};

#undef cookmem_getLargeBinIndex
#undef cookmem_bit2treeIndex

}   // namespace cookmem

#endif  // COOK_MEM_POOL_H
