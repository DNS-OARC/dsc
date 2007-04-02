

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "inX_addr.h"

#if USE_IPV6
static int
is_v4_in_v6(const struct in6_addr *addr)
{
    int i;
    for (i = 0; i < 10; i++)
        if (addr->s6_addr[i] != 0)
            return (0);
    if ((addr->s6_addr[10] != 0xFF) || (addr->s6_addr[11] != 0xFF))
        return (0);
    return 1;
}
#endif


const char *
inXaddr_ntop(inX_addr *a, char *buf, socklen_t len)
{
	const char *p;
#if USE_IPV6
	if (!is_v4_in_v6(&a->in6))
		p = inet_ntop(AF_INET6, &a->in6, buf, len);
	else
#endif
	p =  inet_ntop(AF_INET, &a->_.in4, buf, len);
	if (p)
		return p;
	return "[unprintable]";
}

int
inXaddr_pton(const char *buf, inX_addr *a)
{
	if (strchr(buf, ':'))
		return inet_pton(AF_INET6, buf, a);
	return inet_pton(AF_INET, buf, a);
}

unsigned int
inXaddr_hash(const inX_addr *a)
{
	/* just ignore the upper bits for v6? */
	return ntohl(a->_.in4.s_addr);
}

int
inXaddr_cmp(const inX_addr *a, const inX_addr *b)
{
        if (ntohl(a->_.in4.s_addr) < ntohl(b->_.in4.s_addr))
                return -1;
        if (ntohl(a->_.in4.s_addr) > ntohl(b->_.in4.s_addr))
                return 1;
#if USE_IPV6
	if (is_v4_in_v6(&a->in6))
		return 0;
        if (ntohl(a->_.pad2.s_addr) < ntohl(b->_.pad2.s_addr))
                return -1;
        if (ntohl(a->_.pad2.s_addr) > ntohl(b->_.pad2.s_addr))
                return 1;
        if (ntohl(a->_.pad1.s_addr) < ntohl(b->_.pad1.s_addr))
                return -1;
        if (ntohl(a->_.pad1.s_addr) > ntohl(b->_.pad1.s_addr))
                return 1;
        if (ntohl(a->_.pad0.s_addr) < ntohl(b->_.pad0.s_addr))
                return -1;
        if (ntohl(a->_.pad0.s_addr) > ntohl(b->_.pad0.s_addr))
                return 1;
#endif
	return 0;
}

inX_addr
inXaddr_mask (const inX_addr *a, const inX_addr *mask)
{
	inX_addr masked;
	masked._.in4.s_addr = a->_.in4.s_addr & mask->_.in4.s_addr;
#if USE_IPV6
	if (!is_v4_in_v6(&a->in6)) {
		masked._.pad2.s_addr = a->_.pad2.s_addr & mask->_.pad2.s_addr;
		masked._.pad1.s_addr = a->_.pad1.s_addr & mask->_.pad1.s_addr;
		masked._.pad0.s_addr = a->_.pad0.s_addr & mask->_.pad0.s_addr;
	}
#endif
	return masked;
}

int
inXaddr_version(const inX_addr *a)
{
#if USE_IPV6
	if (!is_v4_in_v6(&a->in6))
		return 6;
#endif
	return 4;
}

int
inXaddr_assign_v4(inX_addr *dst, const struct in_addr *src)
{
	dst->_.in4 = *src;
	return 0;
}

int
inXaddr_assign_v6(inX_addr *dst, const struct in6_addr *src)
{
	dst->in6 = *src;
	return 0;
}


