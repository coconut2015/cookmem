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
#ifndef COOK_EXCEPTION_H
#define COOK_EXCEPTION_H

#include "config.h"

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
    Exception (MemError_et error, const char* msg, const char* file = nullptr, int line = 0)
    : m_error (error),
      m_msg (msg),
      m_file (file),
      m_line (line)
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

    /**
     * Get the file where the exception got thrown.
     *
     * @return  the file where the exception got thrown.
     */
    const char* getFile () const { return m_file; }

    /**
     * Get the line number of where the exception got thrown.
     *
     * @return  the line number of where the exception got thrown.
     */
    int getLine () const { return m_line; }
private:
    const MemError_et   m_error;
    const char*         m_msg;
    const char*         m_file;
    int                 m_line;
};

}   // namespace cookmem

#if COOKMEM_DEBUG > 0
#define COOKMEM_ASSERT(b) do { if (!(b)) throw cookmem::Exception (cookmem::MEM_ERROR_ASSERT, "assertion failure.", __FILE__, __LINE__); } while (0)
#else   // COOKMEM_DEBUG
#define COOKMEM_ASSERT(b)
#endif  // COOKMEM_DEBUG

#endif  // COOK_EXCEPTION_H
