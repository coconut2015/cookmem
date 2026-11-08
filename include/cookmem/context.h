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
#ifndef COOK_MEM_CONTEXT_H
#define COOK_MEM_CONTEXT_H

#include <cstdint>

#include "config.h"
#include "mmaparena.h"
#include "logger.h"
#include "pool.h"

namespace cookmem
{

/**
 * A basic memory context that creates a MemPool based on the arena and logger
 * references user passed in.
 *
 * This class is mostly used in cases when arena or logger that do not have
 * default constructors.
 */
template<class Arena, class Logger, class TreeType = PtrRBTree, class T=void>
class MemContext
{
public:
    /** The type of this memory context. */
    typedef MemContext<Arena, Logger, TreeType, T>  MemCtx;
    /** The type of the memory pool that does the actual work. */
    typedef MemPool<Arena, Logger, TreeType, T>     Pool;

    /** size type */
    typedef typename Pool::size_type        size_type;
    /** pointer difference type */
    typedef typename Pool::difference_type  difference_type;
    /** value type */
    typedef typename Pool::value_type       value_type;
    /** pointer type */
    typedef typename Pool::pointer          pointer;
    /** const pointer type */
    typedef typename Pool::const_pointer    const_pointer;

public:
    /**
     * Constructor.
     *
     * @param   arena
     *          the arena to obtain memory segments from.  It must outlive
     *          this context.
     * @param   logger
     *          the logger for memory activities.  It must outlive this
     *          context.
     * @param   paddingSize
     *          the number of padding bytes (0 to MAX_PADDING_SIZE) added
     *          after each allocation to detect out-of-bounds writes.
     */
    MemContext (Arena&          arena,
                Logger&         logger,
                std::uint8_t    paddingSize = DEFAULT_PADDING_SIZE)
    : m_arena (arena),
      m_logger (logger),
      m_pool (m_arena, m_logger, paddingSize)
    {
    }

    /**
     * Create a new, empty memory context that shares the arena and the
     * logger of another context and uses the same padding size.  No memory
     * is copied.
     *
     * @param   other
     *          the context whose arena and logger are shared.
     */
    MemContext (MemCtx& other)
    : m_arena (other.getArena ()),
      m_logger (other.getLogger ()),
      m_pool (m_arena, m_logger, other.getPaddingSize ())
    {
    }

    virtual ~MemContext()
    {
    }

    /**
     * Allocate memory from the memory context.
     *
     * @param   size
     *          memory request size.
     * @return  the memory region that is at least the request size.
     *          nullptr if the request cannot be satisfied.
     */
    inline T*
    allocate (size_type size) { return m_pool.allocate (size); }

    /**
     * This function reallocate a memory.
     *
     * @param   ptr
     *          the current memory pointer.
     * @param   size
     *          the new size
     * @return  the reallocated pointer.  If the new size cannot be
     *          satisfied, a nullptr is returned and the old pointer
     *          remains valid.
     */
    inline T*
    reallocate (T* ptr, size_type size) { return m_pool.reallocate (ptr, size); }

    /**
     * Allocate n items of certain size.  The memory allocated is zeroed.
     *
     * @param   num
     *          number of elements
     * @param   size
     *          element size
     * @return  memory allocated.
     */
    inline T*
    callocate (size_type num, size_type size) { return m_pool.callocate (num, size); }

    /**
     * Free a piece of memory previously allocated by this memory pool.
     *
     * @param   ptr
     *          a piece of memory to be freed.
     * @param   size
     *          mostly ignored.  It is only used by the memory logger.
     */
    inline void
    deallocate (T* ptr, size_type size = 0) { m_pool.deallocate (ptr, size); }

    /**
     * Validate the internal memory structures.
     */
    inline void
    validate () { m_pool.validate (); }

    /**
     * Release segments that are entirely free.
     */
    inline void
    trimEmptySegments () { m_pool.trimEmptySegments (); }

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
    inline bool
    contains (T* ptr, bool checkUsed = false) { return m_pool.contains (ptr, checkUsed); }

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
    inline size_type
    getSize (T* ptr) { return m_pool.getUserSize (ptr); }

    /**
     * Release all the memory segments held by this MemPool.
     */
    inline void
    releaseAll () { m_pool.releaseAll (); }

    /**
     * Get the memory footprint limit.
     *
     * @return  the memory footprint limit
     */
    inline size_type
    getFootPrintLimit () const { return m_pool.getFootPrintLimit (); }

    /**
     * Set the maximum memory footprint limit.
     *
     * @param   footprintLimit
     *
     */
    inline void
    setFootprintLimit (size_type footprintLimit) { m_pool.setFootprintLimit (footprintLimit); }

    /**
     * Get the current memory footprint.
     *
     * @return  the current memory footprint.
     */
    inline size_type
    getFootprint () const { return m_pool.getFootPrint (); }

    /**
     * Get the maximum memory footprint.
     *
     * @return  the maximum memory footprint.
     */
    inline size_type
    getMaxFootprint () const { return m_pool.getMaxFootprint (); }

    /**
     * Set the number of deallocations between empty-segment trimming passes.
     */
    void
    setTrimEmptySegmentsInterval (size_type interval)
    {
        m_pool.setTrimEmptySegmentsInterval (interval);
    }

    /**
     * Get the number of deallocations between empty-segment trimming passes.
     */
    size_type
    getTrimEmptySegmentsInterval () const
    {
        return m_pool.getTrimEmptySegmentsInterval ();
    }

    /**
     * Check whether or not we are storing the exact user size in the memory.
     *
     * @return  whether or not we are storing the exact user size in the memory.
     */
    inline bool
    isStoringExactSize () const { return m_pool.isStoringExactSize (); }

    /**
     * Set whether or not we should be storing the exact user size in memory.
     *
     * @param   b
     *          the boolean choice
     */
    void
    setStoringExactSize (bool b) { m_pool.setStoringExactSize (b); }

    /**
     * Check whether or not we are padding bytes at the end of the allocated
     * memory.
     *
     * @return  whether or not we are padding bytes at the end of the
     *          allocated memory.
     */
    bool
    isPadding () const { return m_pool.isPadding (); }

    /**
     * Get the number of padding bytes checked after each allocation.
     */
    std::uint8_t
    getPaddingSize () const { return m_pool.getPaddingSize (); }

    /**
     * Get the padding byte.
     *
     * @return  The padding byte.
     */
    std::uint8_t
    getPaddingByte () const { return m_pool.getPaddingByte (); }

    /**
     * Set the padding byte.
     *
     * @param   paddingByte
     *          The padding byte.
     */
    void
    setPaddingByte (std::uint8_t paddingByte) { m_pool.setPaddingByte (paddingByte); }

    /**
     * Get the underlying memory arena.
     *
     * @return  The underlying memory arena.
     */
    inline Arena&
    getArena () { return m_arena;  }

    /**
     * Get the underlying logger.
     *
     * @return  The underlying logger.
     */
    inline Logger&
    getLogger () { return m_logger;  }

    /**
     * Get the underlying memory pool.
     *
     * @return  The underlying memory pool.
     */
    inline Pool&
    getPool () { return m_pool; }

private:
    Arena&      m_arena;
    Logger&     m_logger;
    Pool        m_pool;
};

/**
 * Internal use.
 */
template<class Arena, class Logger>
struct SimpleMemContainer
{
    /** the arena owned by the memory context */
    Arena   arena;
    /** the logger owned by the memory context */
    Logger  logger;

    SimpleMemContainer ()
    : arena (),
      logger ()
    {
    }

    virtual ~SimpleMemContainer ()
    {
    }
};

/**
 * A basic memory context that creates a MemPool using self-contained arena
 * and logger.
 *
 * This class is mostly used in cases when arena or logger both have default
 * constructors.
 */
template<class Arena = MmapArena, class Logger = NoActionMemLogger, class TreeType = PtrRBTree, class T=void>
class SimpleMemContext : private SimpleMemContainer<Arena, Logger>, public MemContext<Arena, Logger, TreeType, T>
{
public:
    /** The type that holds the arena and the logger. */
    typedef SimpleMemContainer<Arena, Logger>   Container;
    /** The type of the base memory context. */
    typedef MemContext<Arena, Logger, TreeType, T>  MemCtx;

    /** size type */
    typedef typename MemCtx::size_type          size_type;
    /** pointer difference type */
    typedef typename MemCtx::difference_type    difference_type;
    /** value type */
    typedef typename MemCtx::value_type         value_type;
    /** pointer type */
    typedef typename MemCtx::pointer            pointer;
    /** const pointer type */
    typedef typename MemCtx::const_pointer      const_pointer;

public:
    /**
     * Constructor.
     *
     * @param   paddingSize
     *          the number of padding bytes (0 to MAX_PADDING_SIZE) added
     *          after each allocation to detect out-of-bounds writes.
     */
    SimpleMemContext (std::uint8_t paddingSize = DEFAULT_PADDING_SIZE)
    : Container (),
      MemCtx (Container::arena, Container::logger, paddingSize)
    {
    }

    /**
     * Create a new, empty memory context with the same padding size as
     * another context.  No memory is copied, and the new context has its
     * own logger.
     *
     * @param   other
     *          the other memory context.
     * @param   shareArena
     *          true to obtain segments from the arena of the other context.
     *          false to use the arena of the new context.
     */
    SimpleMemContext (SimpleMemContext& other, bool shareArena)
    : Container (),
      MemCtx (shareArena ? other.getArena () : Container::arena,
              Container::logger,
              other.getPaddingSize ())
    {
    }

    virtual ~SimpleMemContext ()
    {
    }
};

/**
 * Internal use.
 */
template<class Arena, class Logger>
struct CachedMemContainer
{
    /** the arena that obtains new segments */
    Arena               arena;
    /** the arena that caches released segments on top of arena */
    CachedArena<Arena>  cachedArena;
    /** the logger owned by the memory context */
    Logger              logger;

    CachedMemContainer ()
    : arena (),
      cachedArena (arena),
      logger ()
    {
    }

    virtual ~CachedMemContainer ()
    {
    }
};

/**
 * A cached memory context that creates a MemPool using self-contained arena
 * and logger.
 *
 * This class is mostly used in cases when arena or logger both have default
 * constructors.
 */
template<class Arena = MmapArena, class Logger = NoActionMemLogger, class TreeType = PtrRBTree, class T = void>
class CachedMemContext : private CachedMemContainer<Arena, Logger>, public MemContext<CachedArena<Arena>, Logger, TreeType, T>
{
public:
    /** The type that holds the arenas and the logger. */
    typedef CachedMemContainer<Arena, Logger>   Container;
    /** The type of the base memory context. */
    typedef MemContext<CachedArena<Arena>, Logger, TreeType, T> MemCtx;

    /** size type */
    typedef typename MemCtx::size_type          size_type;
    /** pointer difference type */
    typedef typename MemCtx::difference_type    difference_type;
    /** value type */
    typedef typename MemCtx::value_type         value_type;
    /** pointer type */
    typedef typename MemCtx::pointer            pointer;
    /** const pointer type */
    typedef typename MemCtx::const_pointer      const_pointer;

public:
    /**
     * Constructor.
     *
     * @param   paddingSize
     *          the number of padding bytes (0 to MAX_PADDING_SIZE) added
     *          after each allocation to detect out-of-bounds writes.
     */
    CachedMemContext (std::uint8_t paddingSize = DEFAULT_PADDING_SIZE)
    : Container (),
      MemCtx (Container::cachedArena, Container::logger, paddingSize)
    {
    }

    /**
     * Create a new, empty memory context with the same padding size as
     * another context.  No memory is copied, and the new context has its
     * own logger.
     *
     * @param   other
     *          the other memory context.
     * @param   shareArena
     *          true to share the cached arena of the other context, so that
     *          segments released by either context can be reused by the
     *          other.  false to use the cached arena of the new context.
     */
    CachedMemContext (CachedMemContext& other, bool shareArena)
    : Container (),
      MemCtx (shareArena ? other.getArena () : Container::cachedArena,
              Container::logger,
              other.getPaddingSize ())
    {
    }

    virtual ~CachedMemContext ()
    {
    }
};

}   // namespace cookmem

#endif  // COOK_MEM_CONTEXT_H
