Current Limitations {#limitations}
===================

1. Coalescing of memory chunks is not yet implemented.
2. Locking support is not added.  This feature will not be implemented since
   this project is to be used within a thread.
3. The ability to fill the newly allocated memory or deallocated memory with
   some magic values.  Filling hew newly allocated memory with magic values
   could be added in the future.  However, filling deallocated memory with
   magic values would have limitations due to the internal structures being
   used.

Some of these features will be added later when I have time.
