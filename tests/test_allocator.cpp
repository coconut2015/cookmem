/*
 * Copyright (c) 2021-2026 Heng Yuan
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
#include <cookmem/allocator.h>

#include <list>
#include <vector>

int
main ()
{
    typedef cookmem::CookMemAllocator<int> IntAllocator;
    IntAllocator allocator;

    std::vector<int, IntAllocator> values (allocator);
    values.push_back (42);
    values.push_back (7);

    std::list<int, IntAllocator> linkedValues (allocator);
    linkedValues.push_back (values[0]);
    linkedValues.push_back (values[1]);

    if (values.size () != 2 ||
        linkedValues.size () != 2 ||
        linkedValues.front () != 42 ||
        linkedValues.back () != 7)
    {
        return 1;
    }

    return 0;
}
