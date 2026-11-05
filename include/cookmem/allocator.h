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
 * An allocator backed by a caller-owned CookMem memory context.
 *
 * Allocator copies and rebound allocators share the same context.
 */
template<class T,
         class Arena = MmapArena,
         class Logger = NoActionMemLogger,
         class TreeType = PtrRBTree>
class Allocator
{
public:
    typedef CachedMemContext<Arena, Logger, TreeType> context_type;
    typedef T                       value_type;
    typedef T*                      pointer;
    typedef const T*                const_pointer;
    typedef T&                      reference;
    typedef const T&                const_reference;
    typedef std::size_t             size_type;
    typedef std::ptrdiff_t          difference_type;
    typedef std::false_type         propagate_on_container_copy_assignment;
    typedef std::false_type         propagate_on_container_move_assignment;
    typedef std::false_type         propagate_on_container_swap;
    typedef std::false_type         is_always_equal;

    template<class U>
    struct rebind
    {
        typedef Allocator<U, Arena, Logger, TreeType> other;
    };

    Allocator (const Allocator& other)
    : m_context (other.m_context)
    {
    }

    explicit Allocator (context_type& context)
    : m_context (context)
    {
    }

    template<class U>
    Allocator (const Allocator<U, Arena, Logger, TreeType>& other)
    : m_context (other.m_context)
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

        void* ptr = m_context.allocate (size * sizeof (T));
        if (ptr == nullptr)
        {
            throw std::bad_alloc ();
        }
        return static_cast<pointer> (ptr);
    }

    void
    deallocate (pointer ptr, size_type)
    {
        m_context.deallocate (ptr);
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

    const context_type&
    getContext () const
    {
        return m_context;
    }

    template<class U>
    bool
    operator== (const Allocator<U, Arena, Logger, TreeType>& other) const
    {
        return &m_context == &other.m_context;
    }

    template<class U>
    bool
    operator!= (const Allocator<U, Arena, Logger, TreeType>& other) const
    {
        return !(*this == other);
    }

private:
    template<class, class, class, class>
    friend class Allocator;

    context_type& m_context;
};

}   // namespace cookmem

#endif  // COOK_MEM_ALLOCATOR_H
