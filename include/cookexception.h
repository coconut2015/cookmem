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
#ifndef COOK_EXCEPTION_H
#define COOK_EXCEPTION_H

namespace cookmem
{

/**
 * A simple memory related exception.
 */
class Exception
{
public:
    /**
     * Constructor.
     *
     * @param   msg
     *          the message associated with this exception.
     */
    Exception (const char* msg)
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

}   // namespace cookmem

#ifdef NDEBUG
#define COOKMEM_ASSERT(b)
#else /* NDEBUG */
#define COOKMEM_ASSERT(b) do { if (!(b)) throw Exception("assertion failure."); } while (0)
#endif /* NDEBUG */

#endif  // COOK_EXCEPTION_H
