/* The xmalloc family of functions syslogs an error if the alloc fails. */
void * xmalloc(size_t size);
void * xcalloc(size_t number, size_t size);
void * xrealloc(void *ptr, size_t size);
char * xstrdup(const char *s);
void xfree(void *ptr);

/* The amalloc family of functions allocates from an "arena", optimized for
 * making a large number of small allocations, and then freeing them all at
 * once.  This makes it possible to avoid memory leaks without a lot of
 * tedious tracking of many small allocations.  Like xmalloc, they will syslog
 * an error if the alloc fails.
 * You must call useArena() before using any of these allocators for the first
 * time or after calling freeArena().
 * The only way to free space allocated with these functions is with
 * freeArena(), which quickly frees _everything_ allocated by these functions.
 * afree() is actually a no-op, and arealloc() does not free the original;
 * these will waste space if used heavily.
 */
void useArena();
void freeArena();
void * amalloc(size_t size);
void * acalloc(size_t number, size_t size);
void * arealloc(void *ptr, size_t size);
char * astrdup(const char *s);
void afree(void *ptr);
