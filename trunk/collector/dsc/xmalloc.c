#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <syslog.h>
#if defined (__SVR4) && defined (__sun)
#include <string.h>
#include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include "xmalloc.h"
#include "syslog_debug.h"

/********** xmalloc **********/

void *
xmalloc(size_t size)
{
	void *p = malloc(size);
	if (NULL == p)
	    syslog(LOG_CRIT, "malloc: %s", strerror(errno));
	return p;
}

void *
xcalloc(size_t number, size_t size)
{
	void *p = calloc(number,size);
	if (NULL == p)
	    syslog(LOG_CRIT, "calloc: %s", strerror(errno));
	return p;
}

void *
xrealloc(void *p, size_t size)
{
	p = realloc(p, size);
	if (NULL == p)
	    syslog(LOG_CRIT, "realloc: %s", strerror(errno));
	return p;
}

char *
xstrdup(const char *s)
{
	void *p = strdup(s);
	if (NULL == p)
	    syslog(LOG_CRIT, "strdup: %s", strerror(errno));
	return p;
}

void
xfree(void *p)
{
	free(p);
}


/********** amalloc **********/

typedef struct arena {
    struct arena *prevArena;
    u_char *end;
    u_char *nextAlloc;
} Arena;

Arena *currentArena = NULL;

#define align(size, a) (((size_t)(size) + ((a) - 1)) & ~((a)-1))
#define ALIGNMENT 4
#define HEADERSIZE align(sizeof(Arena), ALIGNMENT)
#define CHUNK_SIZE (1 * 1024 * 1024 + 1024)

static Arena *
newArena(size_t size)
{
    Arena *arena;
    size = align(size, ALIGNMENT);
    arena = malloc(HEADERSIZE + size);
    if (NULL == arena) {
	syslog(LOG_CRIT, "amalloc %d: %s", (int) size, strerror(errno));
	return NULL;
    }
    arena->prevArena = NULL;
    arena->nextAlloc = (u_char*)arena + HEADERSIZE;
    arena->end = arena->nextAlloc + size;
    return arena;
}

void
useArena()
{
    currentArena = newArena(CHUNK_SIZE);
}

void
freeArena()
{
    while (currentArena) {
	Arena *prev = currentArena->prevArena;
	free(currentArena);
	currentArena = prev;
    }
}

void *
amalloc(size_t size)
{
    void *p;
    size = align(size, ALIGNMENT);
    if (currentArena->end - currentArena->nextAlloc <= size) {
	if (size >= (CHUNK_SIZE >> 2)) {
	    /* Create a new dedicated chunk for this large allocation, and
	     * continue to use the current chunk for future smaller
	     * allocations. */
	    Arena *new = newArena(size);
	    new->prevArena = currentArena->prevArena;
	    currentArena->prevArena = new;
	    return new->nextAlloc;
	}
	/* Move on to a new chunk. */
	Arena *new = newArena(CHUNK_SIZE);
	if (NULL == new)
		return NULL;
	new->prevArena = currentArena;
	currentArena = new;
    }
    p = currentArena->nextAlloc;
    currentArena->nextAlloc += size;
    return p;
}

void *
acalloc(size_t number, size_t size)
{
    void *p;
    size *= number;
    p = amalloc(size);
    return memset(p, 0, size);
}

void *
arealloc(void *p, size_t size)
{
    return memcpy(amalloc(size), p, size);
}

char *
astrdup(const char *s)
{
    size_t size = strlen(s) + 1;
    return memcpy(amalloc(size), s, size);
}

void
afree(void *p)
{
    return;
}
