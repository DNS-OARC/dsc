#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <ctype.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#include <syslog.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "byteorder.h"

#define DNS_MSG_HDR_SZ 12
#define RFC1035_MAXLABELSZ 63

static int
rfc1035NameUnpack(const u_char *buf, size_t sz, off_t * off, char *name, int ns)
{
    off_t no = 0;
    unsigned char c;
    size_t len;
    static int loop_detect = 0;
    if (loop_detect > 2)
	return 4;		/* compression loop */
    if (ns <= 0)
	return 4;		/* probably compression loop */
    do {
	if ((*off) >= sz)
	    break;
	c = *(buf + (*off));
	if (c > 191) {
	    /* blasted compression */
	    int rc;
	    unsigned short s;
	    off_t ptr;
	    s = nptohs(buf + (*off));
	    (*off) += sizeof(s);
	    /* Sanity check */
	    if ((*off) >= sz)
		return 1;	/* message too short */
	    ptr = s & 0x3FFF;
	    /* Make sure the pointer is inside this message */
	    if (ptr >= sz)
		return 2;	/* bad compression ptr */
	    if (ptr < DNS_MSG_HDR_SZ)
		return 2;	/* bad compression ptr */
	    loop_detect++;
	    rc = rfc1035NameUnpack(buf, sz, &ptr, name + no, ns - no);
	    loop_detect--;
	    return rc;
	} else if (c > RFC1035_MAXLABELSZ) {
	    /*
	     * "(The 10 and 01 combinations are reserved for future use.)"
	     */
	    return 3;		/* reserved label/compression flags */
	    break;
	} else {
	    (*off)++;
	    len = (size_t) c;
	    if (len == 0)
		break;
	    if (len > (ns - 1))
		len = ns - 1;
	    if ((*off) + len > sz)
		return 4;	/* message is too short */
	    if (no + len + 1 > ns)
		return 5;	/* qname would overflow name buffer */
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
grok_question(const u_char *buf, int len, off_t offset, char *qname, unsigned short *qtype, unsigned short *qclass)
{
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
    *qtype = nptohs(buf + offset);
    *qclass = nptohs(buf + offset + 2);
    offset += 4;
    return offset;
}

static off_t
grok_additional_for_opt_rr(const u_char *buf, int len, off_t offset, dns_message * m)
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
    sometype = nptohs(buf + offset);
    someclass = nptohs(buf + offset + 2);
    if (sometype == T_OPT) {
	m->edns.found = 1;
	m->edns.bufsiz = someclass;
	memcpy(&m->edns.version, buf + offset + 5, 1);
	us = nptohs(buf + offset + 6);
	m->edns.DO = (us >> 15) & 0x01;		/* RFC 3225 */
    }
    /* get rdlength */
    us = nptohs(buf + offset + 8);
    offset += 10;
    if (offset + us > len)
	return 0;
    offset += us;
    return offset;
}

void
handle_dns(const u_char *buf, uint16_t len, transport_message *tm, DMC *dns_message_callback)
{
    unsigned short us;
    off_t offset;
    int qdcount;
    int ancount;
    int nscount;
    int arcount;

    dns_message m[1];

    memset(m, 0, sizeof(m));
    m->tm = tm;
    m->msglen = len;

    if (len < DNS_MSG_HDR_SZ) {
	m->malformed = 1;
	return;
    }
    us = nptohs(buf + 2);
    m->qr = (us >> 15) & 0x01;
    if (0 == m->qr)		/* query */
	m->client_ip_addr = m->tm->src_ip_addr;
    else			/* reply */
	m->client_ip_addr = m->tm->dst_ip_addr;

#if 0
    tc = (us >> 9) & 0x01;
    ra = (us >> 7) & 0x01;
#endif
    m->opcode = (us >> 11) & 0x0F;
    m->rd = (us >> 8) & 0x01;
    m->aa = (us >> 10) & 0x01;
    m->tc = (us >> 9) & 0x01;
    m->rcode = us & 0x0F;

    qdcount = nptohs(buf + 4);
    ancount = nptohs(buf + 6);
    nscount = nptohs(buf + 8);
    arcount = nptohs(buf + 10);

    offset = DNS_MSG_HDR_SZ;

    /*
     * Grab the first question
     */
    if (qdcount > 0 && offset < len) {
	off_t new_offset;
	new_offset = grok_question(buf, len, offset, m->qname, &m->qtype, &m->qclass);
	if (0 == new_offset) {
	    m->malformed = 1;
	    return;
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
    dns_message_callback(m);
}
