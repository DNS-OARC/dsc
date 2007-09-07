
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <syslog.h>
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

static long amalloc_n = 0, amalloc_sz = 0;
static long acalloc_n = 0, acalloc_sz = 0;
static long arealloc_n = 0, arealloc_sz = 0;
static long astrdup_n = 0, astrdup_sz = 0;
static long afree_n = 0;

typedef struct arena {
    struct arena *prevArena;
    u_char *end;
    u_char *nextAlloc;
} Arena;

Arena *currentArena = NULL;

#define align(size, a) (((size_t)(size) + ((a) - 1)) & ~((a)-1))
#define ALIGNMENT 4
#define HEADERSIZE align(sizeof(Arena), ALIGNMENT)
#define MIN_CHUNK_SIZE (1 * 1024 * 1024)

static Arena *
newArena(size_t size)
{
    Arena *arena;
    if (size < MIN_CHUNK_SIZE)
	size = MIN_CHUNK_SIZE;
    size = align(size, ALIGNMENT);
    arena = malloc(HEADERSIZE + size);
    if (NULL == arena)
	syslog(LOG_CRIT, "amalloc: %s", strerror(errno));
    arena->prevArena = NULL;
    arena->nextAlloc = (u_char*)arena + HEADERSIZE;
    arena->end = arena->nextAlloc + size;
    return arena;
}

static void
extendArena(size_t size)
{
    Arena *b = newArena(size);
    b->prevArena = currentArena;
    currentArena = b;
}

void
useArena()
{
    currentArena = newArena(MIN_CHUNK_SIZE);
}

void
amalloc_report()
{
    fprintf(stderr, "### amalloc:  %8ld %8ld\n", amalloc_n, amalloc_sz);
    fprintf(stderr, "### acalloc:  %8ld %8ld\n", acalloc_n, acalloc_sz);
    fprintf(stderr, "### arealloc: %8ld %8ld\n", arealloc_n, arealloc_sz);
    fprintf(stderr, "### astrdup:  %8ld %8ld\n", astrdup_n, astrdup_sz);
    fprintf(stderr, "### afree:    %8ld\n", afree_n);
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

static void *
_amalloc(size_t size)
{
    void *p;
    size = align(size, ALIGNMENT);
    if (currentArena->end - currentArena->nextAlloc <= size)
	extendArena(size);
    p = currentArena->nextAlloc;
    currentArena->nextAlloc += size;
    return p;
}

void *
amalloc(size_t size)
{
    amalloc_sz += size;
    amalloc_n++;
    return _amalloc(size);
}

void *
acalloc(size_t number, size_t size)
{
    size *= number;
    acalloc_sz += size;
    acalloc_n++;
    return memset(_amalloc(size), 0, size);
}

void *
arealloc(void *p, size_t size)
{
    arealloc_sz += size;
    arealloc_n++;
    return memcpy(_amalloc(size), p, size);
}

char *
astrdup(const char *s)
{
    size_t size = strlen(s) + 1;
    astrdup_sz += size;
    astrdup_n++;
    return memcpy(_amalloc(size), s, size);
}

void
afree(void *p)
{
    afree_n++;
}
