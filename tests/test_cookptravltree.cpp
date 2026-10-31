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
#include <cstdlib>
#include <map>
#include <set>

#include <cookptravltree.h>

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

struct Node
{
    std::size_t dummy[7];
};

static int
test1 ()
{
    Node n[20];
    std::size_t size;
    cookmem::PtrAVLTree list;

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
    list.printGraph();

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
    ASSERT_EQ (220, size);
    ASSERT_EQ (&n[9], list.remove (size = 200));
    ASSERT_EQ (&n[18], list.remove (size = 10));
    ASSERT_EQ (&n[0], list.remove (size = 10));
    ASSERT_EQ (&n[6], list.remove (size = 10));
    ASSERT_EQ (&n[3], list.remove (size = 10));

    ASSERT_EQ (true, list.isEmpty ());
    ASSERT_EQ (nullptr, list.remove (size = 0));
    return 0;
}

static int
test2 ()
{
    Node n[1];
    std::size_t size;
    cookmem::PtrAVLTree list;

    list.add (&n[0], 10);
    list.remove ((size = 10));

    return 0;
}

static int
test3 ()
{
    Node n[1];
    cookmem::PtrAVLTree list;

    list.add (&n[0], 10);
    list.remove (&n[0]);

    return 0;
}

static int
test4 ()
{
    Node n[20];
    std::size_t size;
    cookmem::PtrAVLTree list;

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

    return 0;
}

static int
test5 ()
{
    Node searchNodes[11];
    cookmem::PtrAVLTree searchList;
    const std::size_t searchValues[] =
        { 100, 50, 150, 25, 75, 125, 175, 10, 40, 60, 90 };
    for (std::size_t i = 0; i < 11; ++i)
    {
        searchList.add (&searchNodes[i], searchValues[i]);
    }
    std::size_t requested = 95;
    ASSERT_EQ (&searchNodes[0], searchList.remove (requested));
    ASSERT_EQ (std::size_t (100), requested);

    Node n[8];
    cookmem::PtrAVLTree list;

    list.add (&n[0], 20);
    list.add (&n[1], 10);
    list.add (&n[2], 40);
    list.add (&n[3], 5);
    list.add (&n[4], 15);
    list.add (&n[5], 30);
    list.add (&n[6], 50);
    list.add (&n[7], 35);
    list.remove (&n[0]);

    ASSERT_EQ (true, list.contains (&n[1]));
    ASSERT_EQ (true, list.contains (&n[2]));
    ASSERT_EQ (true, list.contains (&n[3]));
    ASSERT_EQ (true, list.contains (&n[4]));
    ASSERT_EQ (true, list.contains (&n[5]));
    ASSERT_EQ (true, list.contains (&n[6]));
    ASSERT_EQ (true, list.contains (&n[7]));
    ASSERT_EQ (false, list.isEmpty ());

    Node duplicates[3];
    cookmem::PtrAVLTree duplicateList;
    std::size_t size = 10;
    duplicateList.add (&duplicates[0], 10);
    duplicateList.add (&duplicates[1], 10);
    duplicateList.add (&duplicates[2], 10);
    ASSERT_EQ (&duplicates[2], duplicateList.remove (size));
    duplicateList.remove (&duplicates[1]);
    ASSERT_EQ (false, duplicateList.contains (&duplicates[1]));
    ASSERT_EQ (true, duplicateList.contains (&duplicates[0]));
    duplicateList.remove (&duplicates[0]);
    ASSERT_EQ (true, duplicateList.isEmpty ());
    return 0;
}

static int
test6 ()
{
    const std::size_t count = 128;
    Node nodes[count];
    bool present[count] = {};
    bool initialized[count] = {};
    std::size_t values[count] = {};
    std::size_t activeCount = 0;
    std::map<std::size_t, std::set<std::size_t> > active;
    cookmem::PtrAVLTree list;
    std::srand (1);

    for (int step = 0; step < 3000; ++step)
    {
        if (activeCount < count && (active.empty () || std::rand () % 2 == 0))
        {
            std::size_t id = static_cast<std::size_t> (std::rand ()) % count;
            while (present[id])
            {
                id = (id + 1) % count;
            }
            std::size_t value = static_cast<std::size_t> (std::rand ()) % 64;
            list.add (&nodes[id], value);
            values[id] = value;
            active[value].insert (id);
            present[id] = true;
            initialized[id] = true;
            ++activeCount;
        }
        else if (std::rand () % 2 == 0)
        {
            std::size_t id = static_cast<std::size_t> (std::rand ()) % count;
            while (!present[id])
            {
                id = (id + 1) % count;
            }
            std::size_t value = values[id];
            list.remove (&nodes[id]);
            active[value].erase (id);
            if (active[value].empty ())
            {
                active.erase (value);
            }
            present[id] = false;
            --activeCount;
        }
        else
        {
            std::size_t requested = static_cast<std::size_t> (std::rand ()) % 70;
            std::map<std::size_t, std::set<std::size_t> >::iterator expected =
                active.lower_bound (requested);
            std::size_t actual = requested;
            Node* removed = static_cast<Node*> (list.remove (actual));
            if (expected == active.end ())
            {
                ASSERT_EQ (nullptr, removed);
            }
            else
            {
                ASSERT_NE (nullptr, removed);
                ASSERT_EQ (expected->first, actual);
                std::size_t id = static_cast<std::size_t> (removed - nodes);
                ASSERT_EQ (1u, expected->second.erase (id));
                present[id] = false;
                --activeCount;
                if (expected->second.empty ())
                {
                    active.erase (expected);
                }
            }
        }

        ASSERT_EQ (active.empty (), list.isEmpty ());
        for (std::size_t id = 0; id < count; ++id)
        {
            if (initialized[id])
            {
                ASSERT_EQ (present[id], list.contains (&nodes[id]));
            }
        }
    }

    return 0;
}

int
main (int argc, const char* argv[])
{
    ASSERT_EQ (0, test1 ());
    ASSERT_EQ (0, test2 ());
    ASSERT_EQ (0, test3 ());
    ASSERT_EQ (0, test4 ());
    ASSERT_EQ (0, test5 ());
    ASSERT_EQ (0, test6 ());
    return 0;
}
