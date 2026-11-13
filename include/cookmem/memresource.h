/*
 * Copyright (c) 2026 Heng Yuan
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
#ifndef COOK_MEM_MEMRESOURCE_H
#define COOK_MEM_MEMRESOURCE_H

// std::pmr::memory_resource needs C++17 and a standard library that has
// <memory_resource>.  COOKMEM_HAS_PMR is 1 when cookmem::MemResource is
// available.  It can be defined to 0 to leave it out.
#ifndef COOKMEM_HAS_PMR
#  if defined(_MSVC_LANG)
#    define COOKMEM_CPLUSPLUS _MSVC_LANG
#  else
#    define COOKMEM_CPLUSPLUS __cplusplus
#  endif
#  if COOKMEM_CPLUSPLUS >= 201703L && defined(__has_include)
#    if __has_include(<memory_resource>)
#      include <memory_resource>
#      if defined(__cpp_lib_memory_resource) && __cpp_lib_memory_resource >= 201603L
#        define COOKMEM_HAS_PMR 1
#      endif
#    endif
#  endif
#  undef COOKMEM_CPLUSPLUS
#  ifndef COOKMEM_HAS_PMR
#    define COOKMEM_HAS_PMR 0
#  endif
#endif

#if COOKMEM_HAS_PMR

#include <cstddef>
#include <memory_resource>
#include <new>

#include "context.h"

namespace cookmem
{

/**
 * A std::pmr::memory_resource that allocates from a memory context, so that
 * the std::pmr containers can use it.  It needs C++17, and is available
 * when COOKMEM_HAS_PMR is 1.
 *
 * Alignments larger than the allocation alignment use allocateAligned ().
 * When the context cannot allocate the memory, std::bad_alloc is thrown, as
 * std::pmr::memory_resource requires.
 *
 * @tparam  Context
 *          a memory context, such as cookmem::MemContext or
 *          cookmem::SimpleMemContext, with void as the value type.
 */
template<class Context>
class MemResource : public std::pmr::memory_resource
{
public:
    /**
     * Constructor.
     *
     * @param   context
     *          the memory context to allocate from.  The caller owns it, and
     *          it must outlive this resource and the memory allocated from
     *          it.
     */
    explicit MemResource (Context& context) noexcept
    : m_context (&context)
    {
    }

    /**
     * Get the memory context.
     *
     * @return  the memory context this resource allocates from.
     */
    Context&
    getContext () const noexcept
    {
        return *m_context;
    }

protected:
    /**
     * Allocate memory from the context.
     *
     * @param   bytes
     *          the size of the memory.
     * @param   alignment
     *          the alignment of the memory, a power of two.
     * @return  the memory allocated.
     * @throws  std::bad_alloc if the memory cannot be allocated.
     */
    void*
    do_allocate (std::size_t bytes, std::size_t alignment) override
    {
        void* ptr = m_context->allocateAligned (bytes, alignment);
        if (ptr == nullptr)
        {
            throw std::bad_alloc ();
        }
        return ptr;
    }

    /**
     * Free memory allocated by do_allocate ().
     *
     * @param   ptr
     *          the memory to free.
     * @param   bytes
     *          the size of the memory, which is only passed to the logger.
     * @param   alignment
     *          the alignment of the memory, which is not needed.
     */
    void
    do_deallocate (void* ptr, std::size_t bytes, std::size_t alignment) override
    {
        (void) alignment;
        m_context->deallocate (ptr, bytes);
    }

    /**
     * Check whether memory allocated by this resource can be freed by
     * another resource.  That is the case when the other resource is a
     * MemResource of the same type for the same context.
     *
     * @param   other
     *          the other resource.
     * @return  true if the two resources use the same context.
     */
    bool
    do_is_equal (const std::pmr::memory_resource& other) const noexcept override
    {
        if (this == &other)
        {
            return true;
        }
#if defined(__cpp_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
        const MemResource* otherResource = dynamic_cast<const MemResource*> (&other);
        return otherResource != nullptr && otherResource->m_context == m_context;
#else
        return false;
#endif
    }

private:
    Context*    m_context;
};

}   // namespace cookmem

#endif  // COOKMEM_HAS_PMR

#endif  // COOK_MEM_MEMRESOURCE_H
