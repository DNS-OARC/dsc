#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <ctype.h>

#include "string_counter.h"
#include "dns_message.h"

#define DNS_MSG_HDR_SZ 12
#define RFC1035_MAXLABELSZ 63

static int
rfc1035NameUnpack(const char *buf, size_t sz, off_t * off, char *name, size_t ns)
{
    off_t no = 0;
    unsigned char c;
    size_t len;
    if (ns <= 0) {
	/* probably compression loop */
	return 4;
    }
    do {
	if ((*off) >= sz)
	    break;
	c = *(buf + (*off));
	if (c > 191) {
	    /* blasted compression */
	    unsigned short s;
	    off_t ptr;
	    memcpy(&s, buf + (*off), sizeof(s));
	    s = ntohs(s);
	    (*off) += sizeof(s);
	    /* Sanity check */
	    if ((*off) >= sz)
		return 1;
	    ptr = s & 0x3FFF;
	    /* Make sure the pointer is inside this message */
	    if (ptr >= sz)
		return 2;
	    return rfc1035NameUnpack(buf, sz, &ptr, name + no, ns - no);
	} else if (c > RFC1035_MAXLABELSZ) {
	    /*
	     * "(The 10 and 01 combinations are reserved for future use.)"
	     */
	    break;
	    return 3;
	} else {
	    (*off)++;
	    len = (size_t) c;
	    if (len == 0)
		break;
	    if (len > (ns - 1))
		len = ns - 1;
	    if ((*off) + len > sz)	/* message is too short */
		return 4;
	    memcpy(name + no, buf + (*off), len);
	    (*off) += len;
	    no += len;
	    *(name + (no++)) = '.';
	}
    } while (c > 0);
    *(name + no - 1) = '\0';
    /* make sure we didn't allow someone to overflow the name buffer */
    assert(no <= ns);
    return 0;
}

dns_message *
handle_dns(const char *buf, int len)
{
    unsigned short us;
    off_t offset;
    char *t;
    int x;
    dns_message *m = calloc(1, sizeof(*m));
    assert(m);
    m->msglen = (unsigned short) len;

    if (len < DNS_MSG_HDR_SZ) {
	free(m);
	return NULL;
    }
    memcpy(&us, buf + 2, 2);
    us = ntohs(us);
    m->qr = (us >> 15) & 0x01;

#if 0
    opcode = (us >> 11) & 0x0F;
    aa = (us >> 10) & 0x01;
    tc = (us >> 9) & 0x01;
    rd = (us >> 8) & 0x01;
    ra = (us >> 7) & 0x01;
#endif
    m->rcode = us & 0x0F;

#if 0
    memcpy(&us, buf + 4, 2);
    qdcount = ntohs(us);

    memcpy(&us, buf + 6, 2);
    ancount = ntohs(us);

    memcpy(&us, buf + 8, 2);
    nscount = ntohs(us);

    memcpy(&us, buf + 10, 2);
    arcount = ntohs(us);
#endif

    /*
     * XXX assume there is exactly one query
     */
    offset = DNS_MSG_HDR_SZ;
    x = rfc1035NameUnpack(buf, len, &offset, m->qname, MAX_QNAME_SZ);
    if (0 != x) {
	free(m);
	return NULL;
    }
    if ('\0' == m->qname[0])
	strcpy(m->qname, ".");
    /* XXX remove special characters from QNAME */
    while ((t = strchr(m->qname, '\n')))
	*t = ' ';
    while ((t = strchr(m->qname, '\r')))
	*t = ' ';
    for (t = m->qname; *t; t++)
	*t = tolower(*t);

    memcpy(&us, buf + offset, 2);
    m->qtype = ntohs(us);
    memcpy(&us, buf + offset + 2, 2);
    m->qclass = ntohs(us);

    return m;
}
