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
#ifndef COOK_UTILS_H
#define COOK_UTILS_H

#include <cstdint>
#include <type_traits>

namespace cookmem
{

/**
 * A simple memory related exception.
 */
class MemException
{
public:
    /**
     * Constructor.
     *
     * @param   msg
     *          the message associated with this exception.
     */
    MemException(const char* msg)
    : m_msg(msg)
    {
    }

    /**
     * Get the exception message.
     *
     * @return  the exception message.
     */
    const char* getMessage() { return m_msg; }
private:
    const char* m_msg;
};

/**
 * This is a utility template to find the smallest integer type that can
 * contain the value specified.
 */
template <int size>
using IntTypeSelector =
    typename std::conditional<size <= 0x7f, std::int8_t,
        typename std::conditional<size <= 0x7fff, std::int16_t,
            typename std::conditional<size <= 0x7fffffff, std::int32_t,
                std::int64_t
            >::type
        >::type
    >::type;

}   // namespace cookmem

#endif  // COOK_UTILS_H
