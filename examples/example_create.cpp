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
#include <iostream>

#include <cookmem.h>

struct Node
{
    Node (int v, Node* n) : value (v), next (n) {}

    int     value;
    Node*   next;
};

int
main (int argc, const char* argv[])
{
    cookmem::SimpleMemContext<> memCtx;

    // create () allocates and constructs, like new.  It throws
    // std::bad_alloc when memory cannot be allocated.
    Node* list = nullptr;
    for (int i = 0; i < 5; ++i)
    {
        list = memCtx.create<Node> (i, list);
    }

    // Expect output 4 3 2 1 0
    for (Node* node = list; node != nullptr; node = node->next)
    {
        std::cout << node->value << " ";
    }
    std::cout << std::endl;

    // destroy () calls the destructor and frees the memory, like delete.
    Node* head = list;
    list = list->next;
    memCtx.destroy (head);

    // The rest of the list does not need to be destroyed one by one.
    // Destroying memCtx, or calling releaseAll (), frees all of it at once,
    // without calling destructors.
    return 0;
}
