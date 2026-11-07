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
    typedef Context context_type;
    typedef std::false_type propagate_on_container_copy_assignment;
    typedef std::false_type propagate_on_container_move_assignment;
    typedef std::false_type propagate_on_container_swap;
    typedef std::false_type is_always_equal;

    explicit MemContextProvider (Context& context)
    : m_context (context)
    {
    }

    Context&
    get () const
    {
        return m_context;
    }

    bool
    operator== (const MemContextProvider& other) const
    {
        return &m_context == &other.m_context;
    }

    bool
    operator!= (const MemContextProvider& other) const
    {
        return !(*this == other);
    }

private:
    Context& m_context;
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
    typedef Context context_type;
    typedef std::true_type propagate_on_container_copy_assignment;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type propagate_on_container_swap;
    typedef std::false_type is_always_equal;

    TLSContextProvider ()
    : m_context (getThreadContext ())
    {
    }

    Context&
    get () const
    {
        return m_context;
    }

    bool
    operator== (const TLSContextProvider& other) const
    {
        return &m_context == &other.m_context;
    }

    bool
    operator!= (const TLSContextProvider& other) const
    {
        return !(*this == other);
    }

private:
    static Context&
    getThreadContext ()
    {
        static thread_local Context context;
        return context;
    }

    Context& m_context;
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
    typedef ContextProvider          context_provider_type;
    typedef typename ContextProvider::context_type context_type;
    typedef T                       value_type;
    typedef T*                      pointer;
    typedef const T*                const_pointer;
    typedef T&                      reference;
    typedef const T&                const_reference;
    typedef std::size_t             size_type;
    typedef std::ptrdiff_t          difference_type;
    typedef typename ContextProvider::propagate_on_container_copy_assignment propagate_on_container_copy_assignment;
    typedef typename ContextProvider::propagate_on_container_move_assignment propagate_on_container_move_assignment;
    typedef typename ContextProvider::propagate_on_container_swap propagate_on_container_swap;
    typedef typename ContextProvider::is_always_equal is_always_equal;

    template<class U>
    struct rebind
    {
        typedef Allocator<U, ContextProvider> other;
    };

    template<class Provider = ContextProvider>
    Allocator (
        typename std::enable_if<
            std::is_default_constructible<Provider>::value,
            int>::type* = nullptr)
    : m_provider ()
    {
    }

    explicit Allocator (context_type& context)
    : m_provider (context)
    {
    }

    explicit Allocator (const ContextProvider& provider)
    : m_provider (provider)
    {
    }

    Allocator (const Allocator& other)
    : m_provider (other.m_provider)
    {
    }

    template<class U>
    Allocator (const Allocator<U, ContextProvider>& other)
    : m_provider (other.m_provider)
    {
    }

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

    void
    deallocate (pointer ptr, size_type)
    {
        m_provider.get ().deallocate (ptr);
    }

    size_type
    max_size () const
    {
        return (std::numeric_limits<size_type>::max) () / sizeof (T);
    }

    template<class U, class... Args>
    void
    construct (U* ptr, Args&&... args)
    {
        ::new (static_cast<void*> (ptr)) U (std::forward<Args> (args)...);
    }

    template<class U>
    void
    destroy (U* ptr)
    {
        ptr->~U ();
    }

    context_type&
    getContext () const
    {
        return m_provider.get ();
    }

    template<class U>
    bool
    operator== (const Allocator<U, ContextProvider>& other) const
    {
        return m_provider == other.m_provider;
    }

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
