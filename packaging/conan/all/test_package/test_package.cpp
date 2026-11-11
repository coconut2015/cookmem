#include <cookmem.h>

#include <iostream>
#include <vector>

int
main ()
{
    cookmem::CachedMemContext<> ctx;
    {
        cookmem::Allocator<int> alloc (ctx);
        std::vector<int, cookmem::Allocator<int> > values (alloc);
        values.push_back (42);
        std::cout << "CookMem " << COOKMEM_VERSION_STRING << ": " << values[0] << std::endl;
    }
    ctx.releaseAllToSystem ();
    return 0;
}
