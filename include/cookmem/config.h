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
#ifndef COOK_MEM_CONFIG_H
#define COOK_MEM_CONFIG_H

#include <cstddef>
#include <cstdint>

#include "version.h"

/**
 * All CookMem classes, functions and constants are in this namespace.
 */
namespace cookmem
{

/**
 * The alignment of every pointer returned by a memory context: twice the
 * size of std::size_t, which is 16 bytes on 64-bit platforms and 8 bytes on
 * 32-bit platforms.  Types that require a larger alignment, such as types
 * declared with alignas(32), are not supported.
 */
const std::size_t ALLOCATION_ALIGNMENT = sizeof (std::size_t) * 2;

/**
 * The number of deallocations between automatic empty-segment trimming passes.
 */
const std::size_t DEFAULT_TRIM_EMPTY_SEGMENTS_INTERVAL = 4095;
/**
 * Default minimum memory segment size.
 *
 * It should be noted that this value needs to be a multiple of 16.
 */
const std::size_t DEFAULT_MIN_SEGMENT_SIZE = 65536;
/**
 * Default number of padding bytes checked after each allocation.
 */
const std::uint8_t DEFAULT_PADDING_SIZE = 0;
/**
 * Maximum number of padding bytes checked after each allocation.
 */
const std::uint8_t MAX_PADDING_SIZE = 8;
/**
 * Default padding byte for strict bounds checking.
 */
const char DEFAULT_PADDING_BYTE = (char)0xcd;

}   // namespace cookmem

#ifndef COOKMEM_DEBUG
#define COOKMEM_DEBUG 0
#endif  // COOKMEM_DEBUG

#endif  // COOK_MEM_CONFIG_H
