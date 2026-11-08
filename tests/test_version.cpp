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
#include <iostream>
#include <sstream>
#include <string>

#include <cookmem.h>
#include "unittest.h"

// The version macros must be usable in preprocessor conditions.
#if !defined(COOKMEM_VERSION) || COOKMEM_VERSION < 0
#error "COOKMEM_VERSION is not usable in preprocessor conditions."
#endif

static int
test1 ()
{
    // The numeric version combines the parts.
    ASSERT_EQ (COOKMEM_VERSION_MAJOR * 10000 + COOKMEM_VERSION_MINOR * 100 + COOKMEM_VERSION_PATCH,
               COOKMEM_VERSION);
    ASSERT_EQ (true, COOKMEM_VERSION_MINOR < 100);
    ASSERT_EQ (true, COOKMEM_VERSION_PATCH < 100);

    // The version string matches the parts.
    std::ostringstream os;
    os << COOKMEM_VERSION_MAJOR << "." << COOKMEM_VERSION_MINOR << "." << COOKMEM_VERSION_PATCH;
    ASSERT_EQ (os.str (), std::string (COOKMEM_VERSION_STRING));

#ifdef COOKMEM_CMAKE_PROJECT_VERSION
    // The version that CMakeLists.txt read from version.h.
    ASSERT_EQ (std::string (COOKMEM_CMAKE_PROJECT_VERSION), std::string (COOKMEM_VERSION_STRING));
#endif

    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, test1 ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }

    return 0;
}
