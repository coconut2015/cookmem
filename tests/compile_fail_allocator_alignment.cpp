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

// This file must NOT compile.  The test_allocator_alignment test builds it
// and passes only if the compiler reports the static_assert in
// cookmem::Allocator::allocate about over-aligned types.

#include <vector>

#include <cookmem/allocator.h>

struct alignas(cookmem::ALLOCATION_ALIGNMENT * 2) OverAligned
{
    char data[cookmem::ALLOCATION_ALIGNMENT * 2];
};

int
main ()
{
    typedef cookmem::Allocator<OverAligned> OverAlignedAllocator;
    OverAlignedAllocator::context_type context;
    OverAlignedAllocator allocator (context);
    std::vector<OverAligned, OverAlignedAllocator> values (allocator);
    values.push_back (OverAligned ());
    return 0;
}
