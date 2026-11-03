/*
 * Copyright (c) 2018 Heng Yuan
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

#include <cookmem/circularlist.h>
#include "unittest.h"

struct Node
{
    Node*   prev;
    Node*   next;
};

static int
test1 ()
{
    Node n1;
    Node n2;
    Node n3;
    Node n4;
    Node n5;
    Node n6;
    Node n7;

    cookmem::CircularList<Node> list;

    ASSERT_EQ (false, list.contains (&n1));

    list.add (&n1);

    ASSERT_EQ (true, list.contains (&n1));

    list.add (&n2);
    list.add (&n3);
    list.add (&n4);
    list.add (&n5);
    list.add (&n6);
    list.add (&n7);

    ASSERT_EQ (true, list.contains (&n1));
    ASSERT_EQ (true, list.contains (&n2));
    ASSERT_EQ (true, list.contains (&n3));
    ASSERT_EQ (true, list.contains (&n4));
    ASSERT_EQ (true, list.contains (&n5));
    ASSERT_EQ (true, list.contains (&n6));
    ASSERT_EQ (true, list.contains (&n7));

    ASSERT_EQ (&n1, list.remove ());
    ASSERT_EQ (false, list.contains (&n1));

    ASSERT_EQ (&n7, list.remove ());
    ASSERT_EQ (&n6, list.remove ());
    ASSERT_EQ (&n5, list.remove ());
    ASSERT_EQ (false, list.contains (&n5));
    ASSERT_EQ (&n4, list.remove ());
    ASSERT_EQ (&n3, list.remove ());
    ASSERT_EQ (&n2, list.remove ());
    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove ());

    list.add (&n1);
    list.add (&n2);
    list.add (&n3);
    list.add (&n4);
    list.add (&n5);
    list.add (&n6);
    list.add (&n7);

    list.remove (&n1);
    list.remove (&n2);
    list.remove (&n3);
    list.remove (&n4);
    ASSERT_EQ (&n7, list.remove ());
    ASSERT_EQ (&n6, list.remove ());
    list.remove (&n5);
    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove ());

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
