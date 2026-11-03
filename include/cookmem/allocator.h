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
#include <utility>

#include "context.h"

namespace cookmem
{

/**
 * An allocator backed by its own CookMem memory context.
 *
 * Allocator copies and rebound allocators have independent contexts.
 */
template<class T,
         class Arena = MmapArena,
         class Logger = NoActionMemLogger,
         class TreeType = PtrRBTree>
class CookMemAllocator
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

    template<class U>
    struct rebind
    {
        typedef CookMemAllocator<U, Arena, Logger, TreeType> other;
    };

    CookMemAllocator ()
    : m_context ()
    {
    }

    CookMemAllocator (const CookMemAllocator&)
    : m_context ()
    {
    }

    explicit CookMemAllocator (bool padding)
    : m_context (padding)
    {
    }

    template<class U>
    CookMemAllocator (const CookMemAllocator<U, Arena, Logger, TreeType>&)
    : m_context ()
    {
    }

    pointer
    allocate (size_type count, const void* = nullptr)
    {
        if (count > max_size ())
        {
            throw std::bad_alloc ();
        }
        if (count == 0)
        {
            return nullptr;
        }

        void* ptr = m_context.allocate (count * sizeof (T));
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

private:
    template<class, class, class, class>
    friend class CookMemAllocator;

    context_type m_context;
};

}   // namespace cookmem

#endif  // COOK_MEM_ALLOCATOR_H
