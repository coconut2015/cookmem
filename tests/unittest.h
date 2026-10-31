/*
 * Copyright (c) 2018-2026 Heng Yuan
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
#ifndef __UNIT_TEST_H__
#define __UNIT_TEST_H__

#include <iostream>
#include <exception>

/**
 * This is a simple unit testing framework.
 */
namespace unittest
{

class UnitTestException : public std::exception
{
public:
    UnitTestException (const char* file, int line)
    :   m_file (file),
        m_line (line)
    {
    }

    const char*
    getFile () const
    {
        return m_file;
    }

    int
    getLine () const
    {
        return m_line;
    }

private:
    const char* m_file;
    int         m_line;
};

class UnitTestAssertException : public UnitTestException
{
public:
    UnitTestAssertException (const char* file, int line)
    :   UnitTestException (file, line)
    {
    }

private:
    const char* m_file;
    int         m_line;
};

template<class T>
inline void
assertEquals (nullptr_t, const T* actual, const char* file, int line)
{
    if (nullptr != actual)
    {
        std::cout << "== Expected: nullptr" << std::endl;
        std::cout << "== Actual  : " << actual << std::endl;

        throw UnitTestAssertException (file, line);
    }
}


template<class T>
inline void
assertEquals (const T& expected, const T& actual, const char* file, int line)
{
    if (expected != actual)
    {
        std::cout << "== Expected: " << expected << std::endl;
        std::cout << "== Actual  : " << actual << std::endl;

        throw UnitTestAssertException (file, line);
    }
}

template<class T>
inline void
assertNotEquals (nullptr_t, const T& actual, const char* file, int line)
{
    if (nullptr == actual)
    {
        std::cout << "== Not expected: nullptr" << std::endl;
        std::cout << "== Actual      : " << actual << std::endl;

        throw UnitTestAssertException (file, line);
    }
}


template<class T>
inline void
assertNotEquals (const T& expected, const T& actual, const char* file, int line)
{
    if (expected == actual)
    {
        std::cout << "== Not expected: " << expected << std::endl;
        std::cout << "== Actual      : " << actual << std::endl;

        throw UnitTestAssertException (file, line);
    }
}

template<class T>
inline void
assertNull (const T* actual, const char* file, int line)
{
    if (actual != nullptr)
    {
        std::cout << "== Expected: nullptr" << std::endl;
        std::cout << "== Actual  : " << actual << std::endl;

        throw UnitTestAssertException (file, line);
    }
}

template<class T>
inline void
assertNotNull (const T* actual, const char* file, int line)
{
    if (actual == nullptr)
    {
        std::cout << "== Not expected: nullptr" << std::endl;

        throw UnitTestAssertException (file, line);
    }
}

typedef void (*TestFunction)();

}       // namespace unittest

#define ASSERT_EQ(e,a) (unittest::assertEquals ((e), (a), __FILE__, __LINE__))
#define ASSERT_NE(e,a) (unittest::assertNotEquals ((e), (a), __FILE__, __LINE__))

#endif  /* __UNIT_TEST_H__ */
