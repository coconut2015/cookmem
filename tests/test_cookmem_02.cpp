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
#include <cstdint>
#include <cstring>
#include <iostream>

#include <cookmem.h>
#include "unittest.h"

/**
 * The usable size of an allocation when the exact size is not stored: the
 * request plus the chunk header, rounded up to ALLOCATION_ALIGNMENT, minus the
 * header.
 * The header is two size_t, so it is 16 bytes on 64-bit platforms and 8
 * bytes on 32-bit platforms.
 */
static std::size_t
roundedSize (std::size_t size)
{
    const std::size_t overhead = 2 * sizeof (std::size_t);
    const std::size_t mask = cookmem::ALLOCATION_ALIGNMENT - 1;
    return ((size + overhead + mask) & ~mask) - overhead;
}

static int
test1 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (0, memCtx.getPaddingSize ());
    ASSERT_EQ (false, memCtx.isStoringExactSize ());

    ASSERT_EQ (0UL, memCtx.getSize (nullptr));

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (roundedSize (30), memCtx.getSize (ptr));
    memCtx.deallocate (ptr);
    ASSERT_EQ (0UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (roundedSize (300), memCtx.getSize (ptr));
    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (roundedSize (3000), memCtx.getSize (ptr));
    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    ASSERT_EQ (roundedSize (30000), memCtx.getSize (ptr));
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (roundedSize (30000), memCtx.getSize (ptr));
    ASSERT_NE (nullptr, ptr);
    ptr = memCtx.allocate (30000);
    ASSERT_EQ (roundedSize (30000), memCtx.getSize (ptr));
    ASSERT_NE (nullptr, ptr);

    return 0;
}

static int
test2 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;

    ASSERT_EQ (false, memCtx.isStoringExactSize ());
    memCtx.setStoringExactSize (true);
    ASSERT_EQ (true, memCtx.isStoringExactSize ());

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 30);
    ASSERT_EQ (30UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 300);
    ASSERT_EQ (300UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (3000UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (30000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    memset (ptr, 0xff, 30000);
    ASSERT_NE (nullptr, ptr);

    ptr = memCtx.allocate (30000);
    ASSERT_EQ (30000UL, memCtx.getSize (ptr));
    memset (ptr, 0xff, 30000);
    ASSERT_NE (nullptr, ptr);

    return 0;
}

static int
test3 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx (4);

    ASSERT_EQ (true, memCtx.isPadding ());
    ASSERT_EQ (4, memCtx.getPaddingSize ());
    ASSERT_EQ (true, memCtx.isStoringExactSize ());

    cookmem::MemContext<cookmem::MallocArena, cookmem::NoActionMemLogger> copiedContext (memCtx);
    ASSERT_EQ (4, copiedContext.getPaddingSize ());

    void* ptr;

    ptr = memCtx.allocate (30);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 30);
    ASSERT_EQ (30UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    ptr = memCtx.allocate (300);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 300);
    ASSERT_EQ (300UL, memCtx.getSize (ptr));

    ptr = memCtx.allocate (3000);
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0xff, 3000);
    ASSERT_EQ (3000UL, memCtx.getSize (ptr));
    memCtx.deallocate (ptr);

    for (size_t size = 30000; size < 30020; ++size)
    {
        ptr = memCtx.allocate (size);
        ASSERT_NE (nullptr, ptr);
        memset (ptr, 0xff, size);
        ASSERT_EQ (size, memCtx.getSize (ptr));
        memCtx.deallocate (ptr);
    }

    return 0;
}

static int
test4 ()
{
    cookmem::SimpleMemContext<cookmem::MallocArena> memCtx;
    void* ptr = memCtx.allocate (8);
    ASSERT_NE (nullptr, ptr);

    // Growing up to the usable size of the chunk stays in place.  The usable
    // size depends on the platform: 16 bytes on 64-bit, 8 bytes on 32-bit.
    const std::size_t usable = memCtx.getSize (ptr);
    ASSERT_EQ (true, usable >= 8);
    void* resized = memCtx.reallocate (ptr, usable);
    ASSERT_EQ (ptr, resized);
    ASSERT_EQ (true, memCtx.contains (resized, true));

    memCtx.deallocate (resized);
    memCtx.deallocate (nullptr);
    memCtx.validate ();
    return 0;
}

/**
 * Resizing in place keeps the padding inside the chunk and the exact size
 * correct, for every combination of old and new sizes.
 */
static int
checkReallocateSizes (std::uint8_t paddingSize)
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx (paddingSize);
    memCtx.setStoringExactSize (true);

    for (std::size_t oldSize = 1; oldSize < 300; oldSize += 7)
    {
        for (std::size_t newSize = 1; newSize < 300; newSize += 5)
        {
            unsigned char* ptr = static_cast<unsigned char*> (memCtx.allocate (oldSize));
            // The neighbor detects writes past the chunk of ptr.
            unsigned char* neighbor = static_cast<unsigned char*> (memCtx.allocate (16));
            ASSERT_NE (nullptr, ptr);
            ASSERT_NE (nullptr, neighbor);
            memset (ptr, 0x11, oldSize);
            memset (neighbor, 0x22, 16);

            ptr = static_cast<unsigned char*> (memCtx.reallocate (ptr, newSize));
            ASSERT_NE (nullptr, ptr);
            ASSERT_EQ (newSize, memCtx.getSize (ptr));
            for (std::size_t i = 0; i < oldSize && i < newSize; ++i)
            {
                ASSERT_EQ (0x11, ptr[i]);
            }
            memset (ptr, 0x33, newSize);
            memCtx.validate ();

            // deallocate () throws if a padding byte was changed.
            memCtx.deallocate (neighbor);
            memCtx.deallocate (ptr);
            memCtx.validate ();
        }
    }
    return 0;
}

static int
test5 ()
{
    ASSERT_EQ (0, checkReallocateSizes (0));
    ASSERT_EQ (0, checkReallocateSizes (1));
    ASSERT_EQ (0, checkReallocateSizes (cookmem::MAX_PADDING_SIZE));
    return 0;
}

/**
 * Requests too large to represent fail without changing the memory.
 */
static int
test6 ()
{
    cookmem::SimpleMemContext<cookmem::MmapArena> memCtx;
    const std::size_t maxSize = ~static_cast<std::size_t> (0);

    char* ptr = static_cast<char*> (memCtx.allocate (100));
    ASSERT_NE (nullptr, ptr);
    memset (ptr, 0x44, 100);
    ASSERT_EQ (nullptr, memCtx.reallocate (ptr, maxSize));
    ASSERT_EQ (nullptr, memCtx.reallocate (ptr, maxSize - 8));
    ASSERT_EQ (true, memCtx.contains (ptr, true));
    ASSERT_EQ (0x44, ptr[99]);
    memCtx.deallocate (ptr);

    // num * size overflows.
    ASSERT_EQ (nullptr, memCtx.callocate (maxSize / 16 + 2, 16));
    ASSERT_EQ (nullptr, memCtx.callocate (16, maxSize / 16 + 2));
    ASSERT_EQ (nullptr, memCtx.callocate (maxSize, maxSize));
    // Zero elements is not an overflow.
    void* zero = memCtx.callocate (0, maxSize);
    ASSERT_NE (nullptr, zero);
    memCtx.deallocate (zero);

    memCtx.validate ();
    return 0;
}

int
main (int argc, const char* argv[])
{
    try
    {
        ASSERT_EQ (0, test1 ());
        ASSERT_EQ (0, test2 ());
        ASSERT_EQ (0, test3 ());
        ASSERT_EQ (0, test4 ());
        ASSERT_EQ (0, test5 ());
        ASSERT_EQ (0, test6 ());
    }
    catch (const unittest::UnitTestException& ex)
    {
        std::cout << "Assertion failed at " << ex.getFile () << ":" << ex.getLine () << std::endl;
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        std::cout << "Exception thrown at " << ex.getFile () << ":"
                  << ex.getLine () << ": " << ex.getMessage () << std::endl;
        return 1;
    }
    return 0;
}
