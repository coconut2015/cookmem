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
// Tests for create () and destroy (), which construct and destroy objects
// in a memory context like new and delete.
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>

#include <cookmem.h>
#include "unittest.h"

static int g_liveObjects = 0;

template<class Context>
static std::size_t
countAllocations (const Context& memCtx)
{
    return memCtx.forEachAllocation ([] (void*, std::size_t) {});
}

struct Point
{
    Point (int x, const std::string& name)
    : m_x (x), m_name (name)
    {
        ++g_liveObjects;
    }

    ~Point ()
    {
        --g_liveObjects;
    }

    int         m_x;
    std::string m_name;
};

struct alignas (64) Aligned
{
    Aligned () { ++g_liveObjects; }
    ~Aligned () { --g_liveObjects; }

    char m_data[64];
};

struct Throwing
{
    explicit Throwing (int value)
    {
        if (value < 0)
        {
            throw std::runtime_error ("negative");
        }
    }

    char m_data[100];
};

struct BaseA
{
    virtual ~BaseA () { --g_liveObjects; }
    int m_a = 1;
};

struct BaseB
{
    virtual ~BaseB () { --g_liveObjects; }
    int m_b = 2;
};

// With two polymorphic bases, a BaseB* points inside the object.
struct Derived : public BaseA, public BaseB
{
    Derived () { g_liveObjects += 3; }
    ~Derived () { --g_liveObjects; }
    char m_data[50];
};

// create () forwards the arguments, and destroy () calls the destructor
// and frees the memory.
static int
test1 ()
{
    cookmem::SimpleMemContext<> memCtx;
    g_liveObjects = 0;

    std::string name ("origin");
    Point* p = memCtx.create<Point> (42, name);
    ASSERT_EQ (42, p->m_x);
    ASSERT_EQ (name, p->m_name);
    ASSERT_EQ (1, g_liveObjects);
    ASSERT_EQ (true, memCtx.contains (p, true));
    ASSERT_EQ (1u, countAllocations (memCtx));

    memCtx.destroy (p);
    ASSERT_EQ (0, g_liveObjects);
    ASSERT_EQ (0u, countAllocations (memCtx));

    // destroy (nullptr) does nothing, like delete.
    Point* nullPoint = nullptr;
    memCtx.destroy (nullPoint);
    ASSERT_EQ (0u, countAllocations (memCtx));
    return 0;
}

// create () honors alignments larger than ALLOCATION_ALIGNMENT.
static int
test2 ()
{
    cookmem::SimpleMemContext<> memCtx;
    g_liveObjects = 0;

    Aligned* objs[10];
    for (int i = 0; i < 10; ++i)
    {
        memCtx.allocate (static_cast<std::size_t> (i * 8 + 1));
        objs[i] = memCtx.create<Aligned> ();
        ASSERT_EQ (0u, reinterpret_cast<std::uintptr_t> (objs[i]) % 64u);
    }
    ASSERT_EQ (10, g_liveObjects);
    for (int i = 0; i < 10; ++i)
    {
        memCtx.destroy (objs[i]);
    }
    ASSERT_EQ (0, g_liveObjects);
    ASSERT_EQ (10u, countAllocations (memCtx));
    return 0;
}

// When the constructor throws, the memory is freed and the exception is
// passed on.
static int
test3 ()
{
    cookmem::SimpleMemContext<> memCtx;

    Throwing* ok = memCtx.create<Throwing> (1);
    ASSERT_EQ (1u, countAllocations (memCtx));

    bool thrown = false;
    try
    {
        memCtx.create<Throwing> (-1);
    }
    catch (std::runtime_error&)
    {
        thrown = true;
    }
    ASSERT_EQ (true, thrown);
    ASSERT_EQ (1u, countAllocations (memCtx));

    memCtx.destroy (ok);
    ASSERT_EQ (0u, countAllocations (memCtx));
    return 0;
}

// When the memory cannot be allocated, create () throws std::bad_alloc
// instead of returning nullptr.
static int
test4 ()
{
    cookmem::SimpleMemContext<> memCtx;
    memCtx.setFootprintLimit (1);
    g_liveObjects = 0;

    bool thrown = false;
    try
    {
        memCtx.create<Point> (1, "none");
    }
    catch (std::bad_alloc&)
    {
        thrown = true;
    }
    ASSERT_EQ (true, thrown);
    ASSERT_EQ (0, g_liveObjects);
    return 0;
}

// destroy () through a polymorphic base class frees the whole object, even
// when the base class pointer does not point to its start.
static int
test5 ()
{
    cookmem::SimpleMemContext<> memCtx;
    g_liveObjects = 0;

    Derived* d1 = memCtx.create<Derived> ();
    Derived* d2 = memCtx.create<Derived> ();
    ASSERT_EQ (6, g_liveObjects);

    BaseB* b = d1;
    ASSERT_NE (static_cast<void*> (b), static_cast<void*> (d1));
    memCtx.destroy (b);
    ASSERT_EQ (3, g_liveObjects);
    ASSERT_EQ (1u, countAllocations (memCtx));

    BaseA* a = d2;
    memCtx.destroy (a);
    ASSERT_EQ (0, g_liveObjects);
    ASSERT_EQ (0u, countAllocations (memCtx));

    // A pointer to const works too, like delete.
    const Point* cp = memCtx.create<Point> (7, "const");
    memCtx.destroy (cp);
    ASSERT_EQ (0, g_liveObjects);
    ASSERT_EQ (0u, countAllocations (memCtx));
    return 0;
}

// releaseAll () frees objects from create () without calling destructors.
static int
test6 ()
{
    cookmem::CachedMemContext<> memCtx;
    g_liveObjects = 0;

    for (int i = 0; i < 100; ++i)
    {
        memCtx.create<Aligned> ();
    }
    ASSERT_EQ (100, g_liveObjects);
    ASSERT_EQ (100u, countAllocations (memCtx));

    memCtx.releaseAll ();
    ASSERT_EQ (100, g_liveObjects);
    ASSERT_EQ (0u, countAllocations (memCtx));
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

    return 0;
}
