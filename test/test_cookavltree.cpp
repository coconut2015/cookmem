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
#include <cookavltree.h>
#include <string>
#include <iostream>

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

static int
test1 ()
{
    cookmem::FixedAVLTree<int, cookmem::ValueComparator<int>, int, 11> tree;

    ASSERT_EQ(0, tree.size());
    ASSERT_EQ(NULL, tree.get(0));

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_EQ(0, *tree.get(0));

    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    ASSERT_EQ(0, *tree.get(0));
    ASSERT_EQ(1, *tree.get(1));
    ASSERT_EQ(2, *tree.get(2));
    ASSERT_EQ(3, *tree.get(3));
    ASSERT_EQ(4, *tree.get(4));
    ASSERT_EQ(5, *tree.get(5));
    ASSERT_EQ(6, *tree.get(6));
    ASSERT_EQ(7, *tree.get(7));
    ASSERT_EQ(8, *tree.get(8));
    ASSERT_EQ(9, *tree.get(9));
    ASSERT_EQ(10, *tree.get(10));

    ASSERT_EQ(NULL, tree.get(11));
    ASSERT_EQ(NULL, tree.get(-1));

    ASSERT_EQ(-1, tree.add(-1, -1));
    ASSERT_EQ(-1, tree.add(11, 11));

    ASSERT_NE(-1, tree.add(9, 11));
    ASSERT_EQ(11, *tree.get(9));

    ASSERT_EQ(11, tree.size());

    ASSERT_NE(-1, tree.remove(0));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(7));
    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(9));
    ASSERT_EQ(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(10));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    ASSERT_NE(-1, tree.remove(10));
    ASSERT_NE(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(7));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(0));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(10));
    ASSERT_NE(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(7));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(0));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(10, 10));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(0, 0));

    ASSERT_EQ(11, tree.size());

    ASSERT_NE(-1, tree.remove(10));
    ASSERT_NE(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(7));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(0));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());
    tree.printGraph ();

    ASSERT_NE(-1, tree.remove(7));
    ASSERT_NE(-1, tree.remove(10));
    ASSERT_NE(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(0));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    ASSERT_NE(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(10));
    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(7));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(0));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    tree.printGraph ();

    ASSERT_NE(-1, tree.remove(8));
    ASSERT_NE(-1, tree.remove(10));
    ASSERT_NE(-1, tree.remove(9));
    ASSERT_NE(-1, tree.remove(0));
    ASSERT_NE(-1, tree.remove(2));
    ASSERT_NE(-1, tree.remove(1));
    ASSERT_NE(-1, tree.remove(3));
    ASSERT_NE(-1, tree.remove(4));
    ASSERT_NE(-1, tree.remove(5));
    ASSERT_NE(-1, tree.remove(6));
    ASSERT_NE(-1, tree.remove(7));

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    tree.clear ();

    ASSERT_EQ(0, tree.size());

    ASSERT_NE(-1, tree.add(0, 0));
    ASSERT_NE(-1, tree.add(1, 1));
    ASSERT_NE(-1, tree.add(2, 2));
    ASSERT_NE(-1, tree.add(3, 3));
    ASSERT_NE(-1, tree.add(4, 4));
    ASSERT_NE(-1, tree.add(5, 5));
    ASSERT_NE(-1, tree.add(6, 6));
    ASSERT_NE(-1, tree.add(7, 7));
    ASSERT_NE(-1, tree.add(8, 8));
    ASSERT_NE(-1, tree.add(9, 9));
    ASSERT_NE(-1, tree.add(10, 10));

    ASSERT_EQ(11, tree.size());

    return 0;
}

static int
test2 ()
{
    cookmem::FixedAVLTree<std::string, cookmem::ObjectComparator<std::string>, std::string, 11> tree;

    ASSERT_EQ(0, tree.size());
    ASSERT_EQ(NULL, tree.get("a"));

    ASSERT_NE(-1, tree.add("a", "a"));
    ASSERT_EQ("a", *tree.get("a"));

    ASSERT_NE(-1, tree.add("b", "b"));
    ASSERT_NE(-1, tree.add("c", "c"));
    ASSERT_NE(-1, tree.add("d", "d"));
    ASSERT_NE(-1, tree.add("e", "e"));
    ASSERT_NE(-1, tree.add("f", "f"));
    ASSERT_NE(-1, tree.add("g", "g"));
    ASSERT_NE(-1, tree.add("h", "h"));
    ASSERT_NE(-1, tree.add("i", "i"));
    ASSERT_NE(-1, tree.add("j", "j"));
    ASSERT_NE(-1, tree.add("k", "k"));

    ASSERT_EQ(11, tree.size());

    ASSERT_EQ("a", *tree.get("a"));
    ASSERT_EQ("b", *tree.get("b"));
    ASSERT_EQ("c", *tree.get("c"));
    ASSERT_EQ("d", *tree.get("d"));
    ASSERT_EQ("e", *tree.get("e"));
    ASSERT_EQ("f", *tree.get("f"));
    ASSERT_EQ("g", *tree.get("g"));
    ASSERT_EQ("h", *tree.get("h"));
    ASSERT_EQ("i", *tree.get("i"));
    ASSERT_EQ("j", *tree.get("j"));
    ASSERT_EQ("k", *tree.get("k"));

    ASSERT_EQ(NULL, tree.get("l"));
    ASSERT_EQ(NULL, tree.get("1"));

    ASSERT_EQ(-1, tree.add("1", "1"));
    ASSERT_EQ(-1, tree.add("l", "l"));

    ASSERT_NE(-1, tree.add("j", "z"));
    ASSERT_EQ("z", *tree.get("j"));

    ASSERT_EQ(11, tree.size());

    ASSERT_NE(-1, tree.remove("a"));
    ASSERT_NE(-1, tree.remove("b"));
    ASSERT_NE(-1, tree.remove("c"));
    ASSERT_NE(-1, tree.remove("d"));
    ASSERT_NE(-1, tree.remove("e"));
    ASSERT_NE(-1, tree.remove("f"));
    ASSERT_NE(-1, tree.remove("g"));
    ASSERT_NE(-1, tree.remove("h"));
    ASSERT_NE(-1, tree.remove("i"));
    ASSERT_NE(-1, tree.remove("j"));
    ASSERT_NE(-1, tree.remove("k"));

    ASSERT_EQ(0, tree.size());
    return 0;
}


int
main(int argc, const char* argv[])
{
    ASSERT_EQ(0, test1 ());
    ASSERT_EQ(0, test2 ());
    return 0;
}
