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
    cookmem::SimpleMemContext<> memCtx;

    void* ptr1 = memCtx.allocate (10);

    // Expect output 16 because the alignment is 16
    std::cout << "Allocated size for ptr1: " << memCtx.getSize (ptr1) << std::endl;

    // Turning on storing the exact user size.
    //
    // It does not use extra memory.  It just finds room in the existing memory
    // to store the information.
    memCtx.setStoringExactSize (true);

    void* ptr2 = memCtx.allocate (10);

    // Expect output 10
    std::cout << "Allocated size for ptr2: " << memCtx.getSize (ptr2) << std::endl;

    // Still expect output 16
    std::cout << "Allocated size for ptr1: " << memCtx.getSize (ptr1) << std::endl;

    // Expect 1 for both pointers.
    //
    // contains first make sure the memory is within memCtx segments,
    // then it would check if the pointer is marked being used.
    //
    std::cout << "ptr1 is used: " << memCtx.contains (ptr1, true) << std::endl;
    std::cout << "ptr2 is used: " << memCtx.contains (ptr2, true) << std::endl;

    memCtx.deallocate (ptr1);
    memCtx.deallocate (ptr2);

    // Expect 0 for both pointers.
    std::cout << "ptr1 is used: " << memCtx.contains (ptr1, true) << std::endl;
    std::cout << "ptr2 is used: " << memCtx.contains (ptr2, true) << std::endl;

    // Both pointers sizes are 0.
    //
    // If you want a perform a fast check if the pointer memory has been
    // freed or not, like in debugger, use getSize ().
    std::cout << "Size for ptr1: " << memCtx.getSize (ptr1) << std::endl;
    std::cout << "Size for ptr2 " << memCtx.getSize (ptr2) << std::endl;

    return 0;
}
