/*
 * Copyright (c) 2025-2026 Heng Yuan
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
#ifndef COOK_MEM_ALLOCATOR_H
#define COOK_MEM_ALLOCATOR_H

#include <cstddef>
#include <functional>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

#include "context.h"

namespace cookmem
{

/**
 * Provides a caller-owned memory context to Allocator.
 *
 * Allocator instances copy this provider, so they continue to refer to the
 * same context without being always equal across different contexts.
 */
template<class Context>
class MemContextProvider
{
public:
    /** The type of the memory context. */
    typedef Context context_type;
    /** Containers keep their own allocator when copy-assigned. */
    typedef std::false_type propagate_on_container_copy_assignment;
    /** Containers keep their own allocator when move-assigned. */
    typedef std::false_type propagate_on_container_move_assignment;
    /** Containers keep their own allocator when swapped. */
    typedef std::false_type propagate_on_container_swap;
    /** Providers are equal only when they refer to the same context. */
    typedef std::false_type is_always_equal;

    /**
     * Constructor.
     *
     * @param   context
     *          the memory context to provide.  The caller owns it, and it
     *          must outlive every allocator that uses this provider.
     */
    explicit MemContextProvider (Context& context)
    : m_context (context)
    {
    }

    /**
     * Get the memory context.
     *
     * @return  the memory context this provider refers to.
     */
    Context&
    get () const
    {
        return m_context.get ();
    }

    /**
     * Check if two providers refer to the same memory context.
     *
     * @param   other
     *          the provider to compare with.
     * @return  true if both providers refer to the same context.
     */
    bool
    operator== (const MemContextProvider& other) const
    {
        return &m_context.get () == &other.m_context.get ();
    }

    /**
     * Check if two providers refer to different memory contexts.
     *
     * @param   other
     *          the provider to compare with.
     * @return  true if the providers refer to different contexts.
     */
    bool
    operator!= (const MemContextProvider& other) const
    {
        return !(*this == other);
    }

private:
    std::reference_wrapper<Context> m_context;
};

/**
 * Provides one shared, default-constructed context per Context, Tag, and thread.
 *
 * Use a distinct Tag type to give different allocator groups separate
 * contexts.
 */
template<class Context, class Tag>
class TLSContextProvider
{
public:
    /** The type of the memory context. */
    typedef Context context_type;
    /** Containers take the allocator of the source when copy-assigned. */
    typedef std::true_type propagate_on_container_copy_assignment;
    /** Containers take the allocator of the source when move-assigned. */
    typedef std::true_type propagate_on_container_move_assignment;
    /** Containers exchange allocators when swapped. */
    typedef std::true_type propagate_on_container_swap;
    /** Providers are equal only when they refer to the same context. */
    typedef std::false_type is_always_equal;

    /**
     * Constructor.  The provider refers to the active context of the
     * current thread: the context of the innermost matching TLSContextScope
     * if there is one, or the persistent thread-local context otherwise.
     */
    TLSContextProvider ()
    : m_context (getContextForAllocator ())
    {
    }

    /**
     * Returns the active context, or the persistent thread context when no
     * scope is active.
     */
    static Context&
    getActiveContext ()
    {
        return activeContext ();
    }

    /**
     * Returns this thread's persistent context, even when a scope is active.
     */
    static Context&
    getThreadContext ()
    {
        return getPersistentThreadContext ();
    }

    /**
     * Get the memory context.
     *
     * @return  the context this provider was bound to when it was
     *          constructed.
     */
    Context&
    get () const
    {
        return m_context.get ();
    }

    /**
     * Check if two providers refer to the same memory context.
     *
     * @param   other
     *          the provider to compare with.
     * @return  true if both providers refer to the same context.
     */
    bool
    operator== (const TLSContextProvider& other) const
    {
        return &m_context.get () == &other.m_context.get ();
    }

    /**
     * Check if two providers refer to different memory contexts.
     *
     * @param   other
     *          the provider to compare with.
     * @return  true if the providers refer to different contexts.
     */
    bool
    operator!= (const TLSContextProvider& other) const
    {
        return !(*this == other);
    }

private:
    template<class, class>
    friend class TLSContextScope;

    static Context&
    activeContext ()
    {
        Context* context = activeContextStorage ();
        return context != nullptr ? *context : getPersistentThreadContext ();
    }

    static Context*&
    activeContextStorage ()
    {
        static thread_local Context* context = nullptr;
        return context;
    }

    static Context&
    getContextForAllocator ()
    {
        return activeContext ();
    }

    static Context&
    getPersistentThreadContext ()
    {
        static thread_local Context context;
        return context;
    }

    std::reference_wrapper<Context> m_context;
};

/**
 * Temporarily provides a function-scope context to default-constructed
 * allocators using the matching TLSContextProvider.
 *
 * Declare the scope before containers that use the context so those
 * containers are destroyed before the context.
 */
template<class Context, class Tag>
class TLSContextScope
{
public:
    /**
     * Constructor.  Creates a new context with its own arena and makes it
     * the active context of the current thread until the scope ends.
     */
    TLSContextScope ()
    : m_context (),
      m_previousContext (TLSContextProvider<Context, Tag>::activeContextStorage ())
    {
        TLSContextProvider<Context, Tag>::activeContextStorage () = &m_context;
    }

    /**
     * Constructor.  Creates a new context based on the persistent
     * thread-local context and makes it the active context of the current
     * thread until the scope ends.
     *
     * @param   shareArena
     *          true to share the arena of the persistent thread-local
     *          context.  With CachedMemContext, segments freed by either
     *          context can then be reused by the other.  false to give the
     *          new context its own arena.
     */
    explicit TLSContextScope (bool shareArena)
    : m_context (TLSContextProvider<Context, Tag>::getPersistentThreadContext (), shareArena),
      m_previousContext (TLSContextProvider<Context, Tag>::activeContextStorage ())
    {
        TLSContextProvider<Context, Tag>::activeContextStorage () = &m_context;
    }

    ~TLSContextScope ()
    {
        TLSContextProvider<Context, Tag>::activeContextStorage () = m_previousContext;
    }

    TLSContextScope (const TLSContextScope&) = delete;
    TLSContextScope& operator= (const TLSContextScope&) = delete;

    /**
     * Get the memory context of this scope.
     *
     * @return  the context that is active while this scope exists.
     */
    Context&
    getContext ()
    {
        return m_context;
    }

private:
    Context   m_context;
    Context*  m_previousContext;
};

/**
 * An STL allocator that allocates through its context provider.
 *
 * Allocator copies and rebound allocators retain the same provider.
 */
template<class T,
         class ContextProvider = MemContextProvider<CachedMemContext<> > >
class Allocator
{
public:
    typedef ContextProvider          context_provider_type;  ///< The type of the context provider.
    typedef typename ContextProvider::context_type context_type;  ///< The type of the memory context.
    typedef T                       value_type;         ///< The type of the allocated objects.
    typedef T*                      pointer;            ///< Pointer to an allocated object.
    typedef const T*                const_pointer;      ///< Const pointer to an allocated object.
    typedef T&                      reference;          ///< Reference to an allocated object.
    typedef const T&                const_reference;    ///< Const reference to an allocated object.
    typedef std::size_t             size_type;          ///< The type of object counts.
    typedef std::ptrdiff_t          difference_type;    ///< The type of pointer differences.
    /** Same as the context provider. */
    typedef typename ContextProvider::propagate_on_container_copy_assignment propagate_on_container_copy_assignment;
    /** Same as the context provider. */
    typedef typename ContextProvider::propagate_on_container_move_assignment propagate_on_container_move_assignment;
    /** Same as the context provider. */
    typedef typename ContextProvider::propagate_on_container_swap propagate_on_container_swap;
    /** Same as the context provider. */
    typedef typename ContextProvider::is_always_equal is_always_equal;

    /**
     * Obtains the allocator type for a different object type that uses the
     * same context provider.
     *
     * @tparam  U
     *          the other object type.
     */
    template<class U>
    struct rebind
    {
        /** The allocator type for objects of type U. */
        typedef Allocator<U, ContextProvider> other;
    };

    /**
     * Default constructor.  It is available only when the context provider
     * is default constructible, such as TLSContextProvider.
     */
    template<class Provider = ContextProvider>
    Allocator (
        typename std::enable_if<
            std::is_default_constructible<Provider>::value,
            int>::type* = nullptr)
    : m_provider ()
    {
    }

    /**
     * Constructor.
     *
     * @param   context
     *          the memory context to allocate from.  It must outlive every
     *          container that uses this allocator.
     */
    explicit Allocator (context_type& context)
    : m_provider (context)
    {
    }

    /**
     * Constructor.
     *
     * @param   provider
     *          the context provider to copy.
     */
    explicit Allocator (const ContextProvider& provider)
    : m_provider (provider)
    {
    }

    /**
     * Copy constructor.  The copy uses the same memory context.
     *
     * @param   other
     *          the allocator to copy.
     */
    Allocator (const Allocator& other)
    : m_provider (other.m_provider)
    {
    }

    /**
     * Converting constructor from an allocator for a different object type.
     * The new allocator uses the same memory context.
     *
     * @param   other
     *          the allocator to copy.
     */
    template<class U>
    Allocator (const Allocator<U, ContextProvider>& other)
    : m_provider (other.m_provider)
    {
    }

    /**
     * Allocate memory for a number of objects.  The objects are not
     * constructed.
     *
     * @param   size
     *          the number of objects.
     * @return  the allocated memory, or nullptr if size is 0.
     * @throws  std::bad_alloc
     *          if the memory cannot be allocated.
     */
    pointer
    allocate (size_type size, const void* = nullptr)
    {
        if (size > (std::numeric_limits<size_type>::max) () / sizeof (T))
        {
            throw std::bad_alloc ();
        }
        if (size == 0)
        {
            return nullptr;
        }

        void* ptr = m_provider.get ().allocate (size * sizeof (T));
        if (ptr == nullptr)
        {
            throw std::bad_alloc ();
        }
        return static_cast<pointer> (ptr);
    }

    /**
     * Deallocate memory obtained from allocate ().  The objects must have
     * been destroyed already.
     *
     * @param   ptr
     *          the memory to deallocate.
     */
    void
    deallocate (pointer ptr, size_type)
    {
        m_provider.get ().deallocate (ptr);
    }

    /**
     * Get the largest number of objects that could be requested.
     *
     * @return  the largest number of objects that allocate () accepts.
     */
    size_type
    max_size () const
    {
        return (std::numeric_limits<size_type>::max) () / sizeof (T);
    }

    /**
     * Construct an object in allocated memory.
     *
     * @param   ptr
     *          where to construct the object.
     * @param   args
     *          the constructor arguments.
     */
    template<class U, class... Args>
    void
    construct (U* ptr, Args&&... args)
    {
        ::new (static_cast<void*> (ptr)) U (std::forward<Args> (args)...);
    }

    /**
     * Destroy an object without deallocating its memory.
     *
     * @param   ptr
     *          the object to destroy.
     */
    template<class U>
    void
    destroy (U* ptr)
    {
        ptr->~U ();
    }

    /**
     * Get the memory context.
     *
     * @return  the memory context this allocator allocates from.
     */
    context_type&
    getContext () const
    {
        return m_provider.get ();
    }

    /**
     * Check if memory allocated by one allocator can be deallocated by the
     * other.
     *
     * @param   other
     *          the allocator to compare with.
     * @return  true if both allocators use the same memory context.
     */
    template<class U>
    bool
    operator== (const Allocator<U, ContextProvider>& other) const
    {
        return m_provider == other.m_provider;
    }

    /**
     * Check if two allocators use different memory contexts.
     *
     * @param   other
     *          the allocator to compare with.
     * @return  true if the allocators use different memory contexts.
     */
    template<class U>
    bool
    operator!= (const Allocator<U, ContextProvider>& other) const
    {
        return !(*this == other);
    }

private:
    template<class, class>
    friend class Allocator;

    ContextProvider m_provider;
};

}   // namespace cookmem

#endif  // COOK_MEM_ALLOCATOR_H
