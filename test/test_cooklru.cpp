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
#include <cooklru.h>
#include <cstdio>

#define ASSERT_EQ(e,v) do { if ((e) != (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)
#define ASSERT_NE(e,v) do { if ((e) == (v)) { std::cout << "Mismatch at line " << __LINE__ << std::endl; return 1; } } while (0)

int main(int argc, const char* argv[])
{
    cookmem::FixedLRU<int, cookmem::ValueComparator<int>, int, 10> lru;

    lru.printLRU ();

    ASSERT_EQ(NULL, lru.get(0));

    lru.add(0, 0);

    ASSERT_EQ(0, *lru.get(0));

    lru.add(1, 1);
    lru.add(2, 2);
    lru.add(3, 3);
    lru.add(4, 4);

    ASSERT_EQ(1, *lru.get(1));
    ASSERT_EQ(2, *lru.get(2));
    ASSERT_EQ(3, *lru.get(3));
    ASSERT_EQ(4, *lru.get(4));

    lru.add(0, 5);
    ASSERT_EQ(1, *lru.get(1));
    ASSERT_EQ(5, *lru.get(0));

    lru.printLRU ();

    lru.add(5, 1);
    lru.add(6, 2);
    lru.add(7, 3);
    lru.add(8, 4);
    lru.add(9, 9);

    ASSERT_NE(-1, lru.add(10, 10));

    lru.printLRU ();

    ASSERT_EQ(NULL, lru.get(2));
    ASSERT_EQ(true, lru.remove(5));
    ASSERT_EQ(NULL, lru.get(5));
    ASSERT_EQ(false, lru.remove(5));

    lru.add(11, 11);
    ASSERT_EQ(3, *lru.get(3));
    ASSERT_EQ(11, 11);

    ASSERT_EQ(true, lru.remove(0));
    ASSERT_EQ(true, lru.remove(1));
    ASSERT_EQ(true, lru.remove(3));
    ASSERT_EQ(true, lru.remove(4));
    ASSERT_EQ(true, lru.remove(6));
    ASSERT_EQ(true, lru.remove(7));
    ASSERT_EQ(true, lru.remove(8));
    ASSERT_EQ(true, lru.remove(9));
    ASSERT_EQ(true, lru.remove(10));
    ASSERT_EQ(true, lru.remove(11));
    ASSERT_EQ(0, lru.size());

    lru.add(1, 1);
    lru.add(2, 2);
    ASSERT_EQ(true, lru.remove(2));
    ASSERT_EQ(true, lru.remove(1));
    ASSERT_EQ(0, lru.size());

    lru.add(1, 1);
    lru.add(2, 2);
    lru.add(3, 3);
    lru.add(4, 4);

    lru.clear ();
    ASSERT_EQ(0, lru.size());
    lru.clear ();
    ASSERT_EQ(0, lru.size());

    lru.add(1, 1);
    lru.add(2, 2);
    lru.add(3, 3);
    lru.add(4, 4);
    lru.add(5, 1);
    lru.add(6, 2);
    lru.add(7, 3);
    lru.add(8, 4);
    lru.add(9, 9);
    lru.add(10, 10);

    ASSERT_EQ(10, lru.size());

    return 0;
}
