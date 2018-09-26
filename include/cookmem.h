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
#ifndef COOK_MEM_H
#define COOK_MEM_H

#include <cstddef>
#include <cstring>

#include <cookmemarena.h>
#include <cookptravltree.h>
#include <cookptrcircularlist.h>
#include <cookutils.h>

namespace cookmem
{

template<class T>
T least_bit(T x)
{
    return ((x) & -(x));
}

template<class T>
T left_bits(T x)
{
    return ((x << 1) | -(x << 1));
}

/* assign tree index for size S to variable I. Use x86 asm if possible  */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define getLargeBinIndex(S, I)\
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
#define getLargeBinIndex(S, I)\
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
#define getLargeBinIndex(S, I)\
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
#define getLargeBinIndex(S, I)\
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
#define compute_bit2idx(X, I)\
{\
  unsigned int J;\
  J = __builtin_ctz(X); \
  I = (BinIndexType)J;\
}

#elif defined (__INTEL_COMPILER)
#define compute_bit2idx(X, I)\
{\
  unsigned int J;\
  J = _bit_scan_forward (X); \
  I = (BinIndexType)J;\
}

#elif defined(_MSC_VER) && _MSC_VER>=1300
#define compute_bit2idx(X, I)\
{\
  unsigned int J;\
  _BitScanForward((DWORD *) &J, X);\
  I = (BinIndexType)J;\
}

#elif USE_BUILTIN_FFS
#define compute_bit2idx(X, I) I = ffs(X)-1

#else
#define compute_bit2idx(X, I)\
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
template<class MemArena, class T=void>
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
    /**
     * Basic memory chunk header
     */
    struct MemChunk
    {
        std::size_t prevFootSize;    /* Size of previous chunk (if free).  */
        std::size_t size;            /* Size */

        inline std::size_t getSize() const
        {
            return size;
        }

        inline void setSize(std::size_t s)
        {
            size = s;
        }

        inline void setFootSize(std::size_t s)
        {
            ((MemChunk*) ((char*) (this) + (s)))->prevFootSize = s;
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

    struct MemSegment
    {
        /**
         * Allocated size.
         */
        std::size_t size;
        /**
         * SLL of segments.
         */
        MemSegment* next;
        /**
         * A simple padding of value 0.  This is to make sure the prevFootSize
         * of memory chunk is 0.
         */
        std::size_t  pad;
        /**
         * Initialize a segment and return the initial memory chunk.
         *
         * @param   segSize
         *          the size of the segment
         * @return  the initial memory chunk inside the segment.
         */
        MemChunk* init (std::size_t segSize)
        {
            size = segSize;

            std::size_t  chunkSize = segSize - SEGMENT_OVERHEAD;
            MemChunk* chunk = (MemChunk*)(((char*)this) + (sizeof(MemSegment) - sizeof(std::size_t)));
            chunk->setSize(chunkSize);
            chunk->setFootSize(chunkSize);
            return chunk;
        }
    };

    typedef unsigned int BinIndexType;
private:
    /**
     * The memory alignment we should aim for.  16 is basically on the
     * safe side.
     */
    static const std::size_t MALLOC_ALIGNMENT = sizeof(std::size_t) * 2;
    /**
     * The mask used to calculate the alignment.
     */
    static const std::size_t ALIGN_MASK = MALLOC_ALIGNMENT - 1;
    /**
     * The number of fixed small segment bins.
     *
     * This number is basically fixed due to the values of SMALLBIN_SHIFT
     * and TREEBIN_SHIFT.
     */
    static const std::size_t NSMALLBINS = 32;
    /**
     * The number of fixed large segment bins.
     */
    static const std::size_t NTREEBINS = 32;
    /**
     * The shift used to calculate the small memory chunk bin.
     */
    static const std::size_t SMALLBIN_SHIFT = 3;
    /**
     * The shift used to calculate the large memory chunk bin.
     */
    static const std::size_t TREEBIN_SHIFT = 8;
    /**
     * The smallest large memory chunk size.
     */
    static const std::size_t MIN_LARGE_SIZE = (1UL << TREEBIN_SHIFT);
    /**
     * The extra bytes needed for a memory chunk.
     */
    static const std::size_t CHUNK_OVERHEAD = sizeof(MemChunk);
    /**
     * The smallest request size that would fit in a large memory chunk.
     */
    static const std::size_t MIN_LARGE_REQUEST = MIN_LARGE_SIZE - CHUNK_OVERHEAD - ALIGN_MASK;
    /**
     * The smallest memory chunk size.
     */
    static const std::size_t MIN_CHUNK_SIZE = ((sizeof(SmallMemChunk) + ALIGN_MASK) & ~ALIGN_MASK);
    /**
     * The largest memory request size allowed.
     */
    static const std::size_t MAX_REQUEST = ((-MIN_CHUNK_SIZE) << 2);                                                                     //////////////
    /**
     * The largest small request size that can fit into the smallest
     * small memory chunk.
     */
    static const std::size_t MIN_REQUEST = MIN_CHUNK_SIZE - CHUNK_OVERHEAD;
    /**
     * The number of extra bytes needed for a segment.
     */
    static const std::size_t SEGMENT_OVERHEAD = (sizeof(MemSegment) - sizeof(std::size_t) + ALIGN_MASK) & ~ALIGN_MASK + sizeof(std::size_t);
    /**
     * The number of frees before attempting to coalesce memory chunks.
     */
    static const std::size_t MAX_RELEASE_CHECK_RATE = 4095;

private:
    /**
     * The memory arena used to allocate / free large memory segments.
     */
    MemArena& m_arena;

    /**
     * The maximum foot print allowed.  0 is no limit.
     */
    std::size_t m_footprintLimit;

    /**
     * SLL of memory segments obtained from arena
     */
    MemSegment  *m_segList;

    std::size_t m_release_checks;

    BinIndexType m_smallMap;
    BinIndexType m_treeMap;

    /**
     * LinkedLists for small chunks
     */
    CircularList<SmallMemChunk> m_smallLists[NSMALLBINS];
    /**
     * AVL trees for large chunks
     */
    PtrAVLTree m_largeTrees[NTREEBINS];

    // memory foot print tracking
    /**
     * The current memory foot print.
     */
    std::size_t m_footprint;
    /**
     * The largest memory foot print at some point of time.
     */
    std::size_t m_maxFootprint;
public:
    /**
     * Constructor
     * @param   arena
     *          memory arena used to allocate memory segments.
     */
    MemPool(MemArena& arena)
    : m_arena(arena),
      m_footprintLimit(0),
      m_segList (nullptr),
      m_release_checks(0),
      m_smallMap(0),
      m_treeMap(0),
      m_smallLists(),
      m_largeTrees(),
      m_footprint(0),
      m_maxFootprint(0)
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
            std::size_t size = seg->size;
            seg = seg->next;
            m_arena.freeSegment(ptr, size);
        }
    }

    /**
     * Set the maximum memory footprint limit.
     *
     * @param   footprintLimit
     *
     */
    void setFootprintLimit (std::size_t footprintLimit)
    {
        m_footprintLimit = footprintLimit;
    }

    /**
     * Get the memory footprint limit.
     *
     * @return  the memory footprint limit
     */
    std::size_t getFootPrintLimit () const
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
    T* allocate(std::size_t size)
    {
        std::size_t  chunkSize;

        if (size < MIN_LARGE_REQUEST)
        {
            BinIndexType binIndex;
            BinIndexType smallBits;
            chunkSize = (size < MIN_REQUEST) ? MIN_CHUNK_SIZE : calcChunkSize(size);
            binIndex = getSmallBinIndex(chunkSize);
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

                chunk = removeSmallChunkList(binIndex);
                COOKMEM_ASSERT(chunk != nullptr && chunk->getSize() == getSmallBinSize (binIndex));

                // mark that this chunk is no longer free.
                chunk->setFootSize(0);

                return chunk2Mem(chunk);
            }
            else
            {
                if (smallBits != 0)
                { /* Use chunk in next nonempty smallbin */
                    MemChunk* chunk;
                    BinIndexType i;
                    BinIndexType leftbits = (smallBits << binIndex) & left_bits(idx2bit(binIndex));
                    BinIndexType leastbit = least_bit(leftbits);
                    compute_bit2idx(leastbit, i);

                    chunk = removeSmallChunkList(i);
                    COOKMEM_ASSERT(chunk != nullptr && chunk->getSize() == getSmallBinSize(i));

                    chunk = splitChunk (chunk, chunkSize);

                    return chunk2Mem(chunk);
                }
            }
        }
        else if (size >= MAX_REQUEST)
        {
            // The request size is too big.
            return nullptr;
        }
        else
        {
            chunkSize = calcChunkSize(size);
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
            chunk = arenaAlloc(chunkSize);

            if (chunk == nullptr)
            {
                return nullptr;
            }
        }

        return chunk2Mem(chunk);
    }

    /**
     * This function reallocate a memory.
     *
     * @param   ptr
     *          the current memory pointer.
     * @param   size
     *          the new size
     * @return  the reallocated pointer.
     */
    T* reallocate(T* ptr, std::size_t size)
    {
        if (ptr == nullptr)
        {
            return allocate(size);
        }

        MemChunk* chunk = mem2Chunk (ptr);
        std::size_t oldChunkSize = chunk->getSize ();

        std::size_t newChunkSize = (size < MIN_REQUEST) ? MIN_CHUNK_SIZE : calcChunkSize(size);

        if (oldChunkSize < newChunkSize)
        {
            // For now we use a simple algorithm that allocate new memory.
            T* newPtr = allocate (size);
            if (newPtr)
            {
                memcpy(newPtr, ptr, oldChunkSize - CHUNK_OVERHEAD);
                deallocate (ptr);
            }
            return newPtr;
        }
        else
        {
            // simply split the chunk
            splitChunk (chunk, newChunkSize);
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
    T* callocate(std::size_t num, std::size_t size)
    {
        std::size_t totalSize = size * num;
        T* ptr = allocate(totalSize);
        if (ptr)
        {
            memset(ptr, 0, totalSize);
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
    void deallocate(T* ptr, std::size_t size = 0)
    {
        if (ptr != nullptr)
        {
            addChunk (mem2Chunk(ptr));
        }
    }

    /**
     * Check if a pointer is within the address space of segments owned
     * by this MemPool.
     *
     * It should be noted that it does not check whether or not the pointer
     * is in use.
     *
     * @param   ptr
     *          memory pointer
     * @return  whether the memory address is in the space of segments owned
     *          by this MemPool.
     */
    bool contains (T* ptr)
    {
        MemSegment* seg = m_segList;
        while (seg)
        {
            if (reinterpret_cast<char*>(ptr) >= reinterpret_cast<char*>(seg) &&
                reinterpret_cast<char*>(ptr) <= (reinterpret_cast<char*>(seg) + seg->size))
            {
                return true;
            }
            seg = seg->next;
        }
        return false;
    }
    /**
     * Release all the memory segments held by this MemPool.
     */
    void releaseAll ()
    {
        MemSegment* seg = m_segList;
        while (seg)
        {
            void* ptr = seg;
            std::size_t size = seg->size;
            seg = seg->next;
            m_arena.freeSegment(ptr, size);
        }
        m_segList = nullptr;
        m_smallMap = 0;
        m_treeMap = 0;
        m_footprint = 0;
        memset(m_largeTrees, 0, sizeof(m_largeTrees));
        memset(m_smallLists, 0, sizeof(m_smallLists));
    }

    /**
     * Get the current memory footprint.
     *
     * @return  the current memory footprint.
     */
    std::size_t getFootprint () const
    {
        return m_footprint;
    }

    /**
     * Get the maximum memory footprint.
     *
     * @return  the maximum memory footprint.
     */
    std::size_t getMaxFootprint () const
    {
        return m_maxFootprint;
    }

private:
    static inline BinIndexType idx2bit(BinIndexType binIndex)
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
    chunk2Mem(MemChunk* chunk)
    {
        return ((T*) ((char*) (chunk) + sizeof(MemChunk)));
    }

    /**
     * Get the memory chunk that contains the user memory address.
     *
     * @param   mem
     *          user memory
     */
    static inline MemChunk*
    mem2Chunk(void* mem)
    {
        return ((MemChunk*) ((char*) (mem) - sizeof(MemChunk)));
    }

    /**
     * From the memory request size, calculate the corresponding chunk size
     * that can hold the request.
     *
     * @param   requestSize
     *          memory request size.
     * @return  the corresponding chunk size.
     */
    static inline std::size_t calcChunkSize(std::size_t requestSize)
    {
        return (((requestSize) + CHUNK_OVERHEAD + ALIGN_MASK) & ~ALIGN_MASK);
    }

    static inline std::size_t getSmallBinSize(BinIndexType binIndex)
    {
        return ((binIndex) << SMALLBIN_SHIFT);
    }

    static inline std::size_t getSmallBinIndex(std::size_t size)
    {
        return (BinIndexType) ((size) >> SMALLBIN_SHIFT);
    }

    static inline bool isSmallChunk(std::size_t size)
    {
        return ((size) >> SMALLBIN_SHIFT) < NSMALLBINS;
    }

    inline PtrAVLTree& treeAt(BinIndexType i)
    {
        return m_largeTrees[i];
    }

    inline MemChunk* treeMalloc (std::size_t size)
    {
        MemChunk* chunk = nullptr;
        std::size_t  actualSize = size;

        BinIndexType binIndex;
        getLargeBinIndex(size, binIndex);

        for (; binIndex < NTREEBINS; ++binIndex)
        {
            PtrAVLTree& tree = treeAt(binIndex);
            if (!tree.isEmpty ())
            {
                chunk = reinterpret_cast<MemChunk*>(tree.remove(actualSize));
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
    inline MemChunk* splitChunk (MemChunk* chunk, std::size_t size)
    {
        // the size of the 2nd half after the split
        std::size_t remainSize = chunk->getSize() - size;

        if (remainSize >= MIN_CHUNK_SIZE)
        {
            MemChunk* remainChunk = (MemChunk*)((char*)chunk + size);

            chunk->setSize(size);
            chunk->setFootSize(0);

            remainChunk->setSize(remainSize);
            remainChunk->setFootSize(remainSize);

            addChunk(remainChunk);
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
    MemChunk* arenaAlloc(std::size_t chunkSize)
    {
        std::size_t estSize;             /* allocation size */

        estSize = chunkSize + SEGMENT_OVERHEAD;
        if (estSize <= chunkSize)
        {
            return nullptr;         /* wraparound */
        }

        /*
         * Check user specified foot print limit
         */
        if (m_footprintLimit != 0)
        {
            std::size_t fp = m_footprint + estSize;
            if (fp <= m_footprint || fp > m_footprintLimit)
                return nullptr;
        }

        /*
         * Request memory segments from arena.
         */
        std::size_t segSize = estSize;
        MemSegment* seg = (MemSegment*)m_arena.getSegment(segSize);

        /*
         * Initialize the segment obtained.
         */
        if (seg)
        {

            if ((m_footprint += segSize) > m_maxFootprint)
                m_maxFootprint = m_footprint;

            MemChunk* chunk = seg->init(segSize);

            if (m_segList == nullptr)
            {
                m_segList = seg;
                seg->next = nullptr;

                m_release_checks = MAX_RELEASE_CHECK_RATE;
            }
            else
            {
                seg->next = m_segList;
                m_segList = seg;
            }

            return splitChunk(chunk, chunkSize);
        }
        return nullptr;
    }

    inline CircularList<SmallMemChunk>& getSmallChunkList(BinIndexType binIndex)
    {
        return m_smallLists[binIndex];
    }

    inline void addSmallChunk(MemChunk* chunk)
    {
        BinIndexType I = getSmallBinIndex(chunk->getSize());

        CircularList<SmallMemChunk>& freeList = getSmallChunkList(I);
        freeList.add ((SmallMemChunk*)chunk);
        if (!isSmallMapMarked(I))
            markSmallMap(I);
    }

    inline MemChunk* removeSmallChunkList(BinIndexType binIndex)
    {
        CircularList<SmallMemChunk>& freeList = getSmallChunkList(binIndex);
        SmallMemChunk* chunk = freeList.remove();
        if (freeList.isEmpty())
        {
            clearSmallMap(binIndex);
        }
        return chunk;
    }

    inline void removeSmallChunk(MemChunk* chunk)
    {
        BinIndexType binIndex = getSmallBinIndex(chunk->getSize ());

        CircularList<SmallMemChunk>& freeList = getSmallChunkList(binIndex);
        if (freeList.remove((SmallMemChunk*)chunk))
        {
            clearSmallMap(binIndex);
        }
    }

    inline void markSmallMap(BinIndexType binIndex)
    {
        m_smallMap |= idx2bit(binIndex);
    }

    inline void clearSmallMap(BinIndexType binIndex)
    {
        m_smallMap &= ~idx2bit(binIndex);
    }

    inline bool isSmallMapMarked(BinIndexType binIndex)
    {
        return m_smallMap & idx2bit(binIndex);
    }

    inline void markTreeMap(BinIndexType binIndex)
    {
        m_treeMap |= idx2bit(binIndex);
    }
    inline void clearTreeMap(BinIndexType binIndex)
    {
        m_treeMap &= ~idx2bit(binIndex);
    }
    inline bool isTreeMapMarked(BinIndexType binIndex)
    {
        return m_treeMap & idx2bit(binIndex);
    }

    void addLargeChunk(MemChunk* chunk)
    {
        BinIndexType treeBinIndex;
        getLargeBinIndex(chunk->getSize(), treeBinIndex);
        PtrAVLTree& tree = treeAt(treeBinIndex);
        if (!isTreeMapMarked(treeBinIndex))
        {
            markTreeMap(treeBinIndex);
        }
        tree.add(chunk, chunk->getSize());
    }

    inline void removeLargeChunk(MemChunk* chunk)
    {
        BinIndexType treeBinIndex;
        getLargeBinIndex(chunk->getSize(), treeBinIndex);
        PtrAVLTree& tree = treeAt(treeBinIndex);

        tree.remove(chunk);
        if (tree.isEmpty())
        {
            clearTreeMap(treeBinIndex);
        }
    }

    inline void addChunk(MemChunk* chunk)
    {
        if (isSmallChunk(chunk->getSize()))
        {
            addSmallChunk(chunk);
        }
        else
        {
            addLargeChunk(chunk);
        }
    }

    inline void removeChunk(MemChunk* chunk)
    {
        if (isSmallChunk(chunk->getSize()))
            removeSmallChunk(chunk);
        else
        {
            removeLargeChunk(chunk);
        }
    }
};

#undef getLargeBinIndex

}   // namespace heng yuan

#endif  // COOK_MEM_H
