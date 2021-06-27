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
#ifndef COOK_MEM_CONTEXT_H
#define COOK_MEM_CONTEXT_H

#include "cookmmaparena.h"
#include "cookmemlogger.h"
#include "cookmempool.h"

namespace cookmem
{

/**
 * A basic memory context that creates a MemPool based on the arena and logger
 * references user passed in.
 *
 * This class is mostly used in cases when arena or logger that do not have
 * default constructors.
 */
template<class Arena, class Logger, class T=void>
class MemContext
{
public:
    typedef MemContext<Arena, Logger, T>    MemCtx;
    typedef MemPool<Arena, Logger, T>       Pool;

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
    MemContext (Arena&  arena,
                Logger& logger,
                bool    padding = false)
    : m_arena (arena),
      m_logger (logger),
      m_pool (m_arena, m_logger, padding)
    {
    }

    MemContext (MemCtx& other)
    : m_arena (other.getArena ()),
      m_logger (other.getLogger ()),
      m_pool (m_arena, m_logger, other.isPadding ())
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
    allocate (std::size_t size) { return m_pool.allocate (size); }

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
    reallocate (T* ptr, std::size_t size) { return m_pool.reallocate (ptr, size); }

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
    callocate (std::size_t num, std::size_t size) { return m_pool.callocate (num, size); }

    /**
     * Free a piece of memory previously allocated by this memory pool.
     *
     * @param   ptr
     *          a piece of memory to be freed.
     * @param   size
     *          mostly ignored.  It is only used by the memory logger.
     */
    inline void
    deallocate (T* ptr, std::size_t size = 0) { m_pool.deallocate (ptr, size); }

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
    size_type
    getSize (T* ptr)
    {
        return m_pool.getUserSize (ptr);
    }

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
    inline std::size_t
    getFootPrintLimit () const { return m_pool.getFootPrintLimit (); }

    /**
     * Set the maximum memory footprint limit.
     *
     * @param   footprintLimit
     *
     */
    inline void
    setFootprintLimit (std::size_t footprintLimit) { m_pool.setFootprintLimit (footprintLimit); }

    /**
     * Get the current memory footprint.
     *
     * @return  the current memory footprint.
     */
    inline std::size_t
    getFootprint () const { return m_pool.getFootPrint (); }

    /**
     * Get the maximum memory footprint.
     *
     * @return  the maximum memory footprint.
     */
    inline std::size_t
    getMaxFootprint () const { return m_pool.getMaxFootprint (); }

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
    Arena   arena;
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
template<class Arena = MmapArena, class Logger = NoActionMemLogger, class T=void>
class SimpleMemContext : private SimpleMemContainer<Arena, Logger>, public MemContext<Arena, Logger, T>
{
public:
    typedef SimpleMemContainer<Arena, Logger>   Container;
    typedef MemContext<Arena, Logger, T>        MemCtx;

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
    SimpleMemContext (bool padding = false)
    : Container (),
      MemCtx (Container::arena, Container::logger, padding)
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
    Arena               arena;
    CachedArena<Arena>  cachedArena;
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
template<class Arena = MmapArena, class Logger = NoActionMemLogger, class T = void>
class CachedMemContext : private CachedMemContainer<Arena, Logger>, public MemContext<CachedArena<Arena>, Logger, T>
{
public:
    typedef CachedMemContainer<Arena, Logger>   Container;
    typedef MemContext<CachedArena<Arena>, Logger, T>   MemCtx;

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
    CachedMemContext (bool padding = false)
    : Container (),
      MemCtx (Container::cachedArena, Container::logger, padding)
    {
    }

    virtual ~CachedMemContext ()
    {
    }
};

}   // namespace cookmem

#endif  // COOK_MEM_CONTEXT_H
