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
#include <iostream>

#include <cookmem.h>

int
main (int argc, const char* argv[])
{
    try
    {
        cookmem::CachedMemContext<> memCtx (true);

        void* ptr = memCtx.allocate (10);

        // Expect output 10 because turning on padding also turning on
        // the option to store the exact allocated size.
        std::cout << "Allocated size for ptr: " << memCtx.getSize (ptr) << std::endl;

        // intentionally writing pass the allocated size.
        memset (ptr, 0xff, 11);

        memCtx.deallocate (ptr);
    }
    catch (cookmem::Exception& ex)
    {
        std::cout << ex.getMessage () << std::endl;
    }

    return 0;
}
