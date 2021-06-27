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

typedef enum
{
    MEM_ERROR_GENERAL,      // general error
    MEM_ERROR_ASSERT,       // assertion failure
    MEM_ERROR_DOUBLE_FREE,  // freeing an already free pointer
    MEM_ERROR_PADDING,      // padding byte error
}  MemError_et;

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
    Exception (MemError_et error, const char* msg)
    : m_error (error),
      m_msg (msg)
    {
    }

    /**
     * Get the exception error.
     *
     * @return  the exception error.
     */
    const MemError_et getError () const { return m_error; }

    /**
     * Get the exception message.
     *
     * @return  the exception message.
     */
    const char* getMessage () const { return m_msg; }
private:
    const MemError_et   m_error;
    const char*         m_msg;
};

}   // namespace cookmem

#ifdef NDEBUG
#define COOKMEM_ASSERT(b)
#else /* NDEBUG */
#define COOKMEM_ASSERT(b) do { if (!(b)) throw Exception (cookmem::MEM_ERROR_ASSERT, "assertion failure."); } while (0)
#endif /* NDEBUG */

#endif  // COOK_EXCEPTION_H
