#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <ctype.h>
#include <arpa/nameser.h>

#include "string_counter.h"
#include "dns_message.h"

#define DNS_MSG_HDR_SZ 12
#define RFC1035_MAXLABELSZ 63

static int
rfc1035NameUnpack(const char *buf, size_t sz, off_t * off, char *name, int ns)
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
    if (no > 0)
        *(name + no - 1) = '\0';
    /* make sure we didn't allow someone to overflow the name buffer */
    assert(no <= ns);
    return 0;
}

static off_t
grok_question(const char *buf, int len, off_t offset, char *qname, unsigned short *qtype, unsigned short *qclass)
{
    unsigned short us;
    char *t;
    int x;
    x = rfc1035NameUnpack(buf, len, &offset, qname, MAX_QNAME_SZ);
    if (0 != x)
	return 0;
    if ('\0' == *qname)
	strcpy(qname, ".");
    /* XXX remove special characters from QNAME */
    while ((t = strchr(qname, '\n')))
	*t = ' ';
    while ((t = strchr(qname, '\r')))
	*t = ' ';
    for (t = qname; *t; t++)
	*t = tolower(*t);
    if (offset + 4 > len)
	return 0;
    memcpy(&us, buf + offset, 2);
    *qtype = ntohs(us);
    memcpy(&us, buf + offset + 2, 2);
    *qclass = ntohs(us);
    offset += 4;
    return offset;
}

static off_t
grok_additional_for_opt_rr(const char *buf, int len, off_t offset, dns_message * m)
{
    int x;
    unsigned short sometype;
    unsigned short someclass;
    unsigned short us;
    char somename[MAX_QNAME_SZ];
    x = rfc1035NameUnpack(buf, len, &offset, somename, MAX_QNAME_SZ);
    if (0 != x)
	return 0;
    if (offset + 10 > len)
	return 0;
    memcpy(&us, buf + offset, 2);
    sometype = ntohs(us);
    memcpy(&us, buf + offset + 2, 2);
    someclass = ntohs(us);
    if (sometype == T_OPT) {
	m->edns.found = 1;
	memcpy(&m->edns.version, buf + offset + 5, 1);
	memcpy(&us, buf + 6, 2);
	us = ntohs(us);
	m->edns.d0 = (us >> 15) & 0x01;
    }
    /* get rdlength */
    memcpy(&us, buf + offset + 8, 2);
    us = ntohs(us);
    offset += 10;
    if (offset + us > len)
	return 0;
    offset += us;
    return offset;
}

dns_message *
handle_dns(const char *buf, int len)
{
    unsigned short us;
    off_t offset;
    int qdcount;
    int ancount;
    int nscount;
    int arcount;
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

    memcpy(&us, buf + 4, 2);
    qdcount = ntohs(us);
    memcpy(&us, buf + 6, 2);
    ancount = ntohs(us);
    memcpy(&us, buf + 8, 2);
    nscount = ntohs(us);
    memcpy(&us, buf + 10, 2);
    arcount = ntohs(us);

    offset = DNS_MSG_HDR_SZ;

    /*
     * Grab the first question
     */
    if (qdcount > 0 && offset < len) {
	off_t new_offset;
	new_offset = grok_question(buf, len, offset, m->qname, &m->qtype, &m->qclass);
	if (0 == new_offset) {
	    free(m);
	    return NULL;
	}
	offset = new_offset;
	qdcount--;
    }
    assert(offset <= len);
    /*
     * Gobble up subsequent questions, if any
     */
    while (qdcount > 0 && offset < len) {
	off_t new_offset;
	char t_qname[MAX_QNAME_SZ];
	unsigned short t_qtype;
	unsigned short t_qclass;
	new_offset = grok_question(buf, len, offset, t_qname, &t_qtype, &t_qclass);
	if (0 == new_offset) {
	    /*
	     * point offset to the end of the buffer to avoid any subsequent processing
	     */
	    offset = len;
	    break;
	}
	offset = new_offset;
	qdcount--;
    }
    assert(offset <= len);

    if (arcount > 0 && offset < len) {
	off_t new_offset;
	new_offset = grok_additional_for_opt_rr(buf, len, offset, m);
	if (0 == new_offset) {
	    offset = len;
	} else {
	    offset = new_offset;
	}
	arcount--;
    }
    assert(offset <= len);
    return m;
}
