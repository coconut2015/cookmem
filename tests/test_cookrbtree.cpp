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
#include <iostream>

#include <cookrbtree.h>

#include "unittest.h"

static void
test1 ()
{
    cookmem::PtrRBNode n[20];
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_EQ (false, list.contains (&n[i]));
    }

    list.add (&n[0], 10);
    list.add (&n[1], 20);
    list.add (&n[2], 30);
    list.add (&n[3], 40);
    list.add (&n[4], 50);
    list.add (&n[5], 20);
    list.add (&n[6], 40);
    list.add (&n[7], 80);
    list.add (&n[8], 150);
    list.add (&n[9], 250);
    list.add (&n[10], 60);
    list.add (&n[11], 220);
    list.add (&n[12], 330);
    list.add (&n[13], 440);
    list.add (&n[14], 550);
    list.add (&n[15], 320);
    list.add (&n[16], 340);
    list.add (&n[17], 430);
    list.add (&n[18], 10);
    list.add (&n[19], 20);

    ASSERT_EQ (false, list.isEmpty ());
    // list.printGraph();

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_EQ (true, list.contains (&n[i]));
    }

    ASSERT_EQ (&n[8], list.remove (size = 100));
    ASSERT_EQ (false, list.contains (&n[8]));

    ASSERT_EQ (&n[14], list.remove (size = 550));
    ASSERT_EQ (false, list.contains (&n[14]));

    ASSERT_EQ (&n[19], list.remove (size = 20));
    ASSERT_EQ (false, list.contains (&n[19]));
    ASSERT_EQ (&n[5], list.remove (size = 20));
    ASSERT_EQ (false, list.contains (&n[5]));
    ASSERT_EQ (&n[1], list.remove (size = 20));
    ASSERT_EQ (false, list.contains (&n[1]));
    ASSERT_EQ (&n[7], list.remove (size = 80));
    ASSERT_EQ (&n[2], list.remove (size = 25));
    ASSERT_EQ (nullptr, list.remove (size = 10000));
    ASSERT_EQ (&n[15], list.remove (size = 300));
    ASSERT_EQ (&n[12], list.remove (size = 300));
    ASSERT_EQ (&n[16], list.remove (size = 300));
    ASSERT_EQ (&n[17], list.remove (size = 300));
    ASSERT_EQ (&n[4], list.remove (size = 45));
    ASSERT_EQ (&n[10], list.remove (size = 45));
    ASSERT_EQ (&n[13], list.remove (size = 300));
    ASSERT_EQ (&n[11], list.remove (size = 200));
    ASSERT_EQ (std::size_t (220), size);
    ASSERT_EQ (&n[9], list.remove (size = 200));
    ASSERT_EQ (&n[18], list.remove (size = 10));
    ASSERT_EQ (&n[0], list.remove (size = 10));
    ASSERT_EQ (&n[6], list.remove (size = 10));
    ASSERT_EQ (&n[3], list.remove (size = 10));

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));
}

static void
test2 ()
{
    cookmem::PtrRBNode n[1];
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    list.add (&n[0], 10);
    list.remove ((size = 10));
}

static void
test3 ()
{
    cookmem::PtrRBNode n[1];
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    list.add (&n[0], 10);
    list.remove (&n[0]);
}

static void
test4 ()
{
    cookmem::PtrRBNode n[20];
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_EQ (false, list.contains (&n[i]));
    }

    list.add (&n[0], 10);
    list.add (&n[1], 20);
    list.add (&n[2], 30);
    list.add (&n[3], 40);
    list.add (&n[4], 50);
    list.add (&n[5], 20);
    list.add (&n[6], 40);
    list.add (&n[7], 80);
    list.add (&n[8], 150);
    list.add (&n[9], 250);
    list.add (&n[10], 60);
    list.add (&n[11], 220);
    list.add (&n[12], 330);
    list.add (&n[13], 440);
    list.add (&n[14], 550);
    list.add (&n[15], 320);
    list.add (&n[16], 340);
    list.add (&n[17], 430);
    list.add (&n[18], 10);
    list.add (&n[19], 20);

    list.remove (&n[8]);
    list.remove (&n[14]);
    list.remove (&n[19]);
    list.remove (&n[5]);
    list.remove (&n[1]);
    list.remove (&n[7]);
    list.remove (&n[2]);
    list.remove (&n[15]);
    list.remove (&n[12]);
    list.remove (&n[16]);
    list.remove (&n[17]);
    list.remove (&n[4]);
    list.remove (&n[10]);
    list.remove (&n[13]);
    list.remove (&n[11]);
    list.remove (&n[9]);
    list.remove (&n[18]);
    list.remove (&n[0]);
    list.remove (&n[6]);
    list.remove (&n[3]);

    ASSERT_EQ (true, list.isEmpty ());
}

static void
test5 ()
{
    cookmem::PtrRBNode n[3];
    cookmem::PtrRBNode outsider;
    std::size_t size;
    cookmem::RedBlackTree<cookmem::PtrRBNode> list;

    list.add (&n[0], 10);
    list.add (&n[1], 10);
    list.add (&n[2], 10);
    outsider.init (10, nullptr);
    ASSERT_EQ (false, list.contains (&outsider));

    list.remove (&n[0]);
    ASSERT_EQ (false, list.contains (&n[0]));
    ASSERT_EQ (true, list.contains (&n[1]));
    ASSERT_EQ (true, list.contains (&n[2]));

    ASSERT_EQ (&n[1], list.remove (size = 0));
    ASSERT_EQ (std::size_t (10), size);
    ASSERT_EQ (false, list.contains (&n[1]));
    list.remove (&n[2]);
    ASSERT_EQ (true, list.isEmpty ());
}

unittest::TestFunction s_tests[] =
{
    test1,
    test2,
    test3,
    test4,
    test5,
};

int
main (int argc, const char* argv[])
{
    try
    {
        std::size_t numTests = sizeof (s_tests) / sizeof (unittest::TestFunction);
        for (std::size_t i = 0; i < numTests; ++i)
        {
            s_tests[i] ();
        }
    }
    catch (unittest::UnitTestException& ex)
    {
        printf ("Exception thrown at %s:%d.\n", ex.getFile (), ex.getLine ());
        return 1;
    }
    catch (cookmem::Exception& ex)
    {
        printf ("Exception thrown at %s:%d.\n", ex.getFile (), ex.getLine ());
        return 1;
    }
    return 0;
}
