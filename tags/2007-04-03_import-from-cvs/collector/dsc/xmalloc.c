
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <syslog.h>
#include "xmalloc.h"


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

char *
xstrdup(const char *s)
{
	void *p = strdup(s);
	if (NULL == p)
	    syslog(LOG_CRIT, "strdup: %s", strerror(errno));
	return p;
}
