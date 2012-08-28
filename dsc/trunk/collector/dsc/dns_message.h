#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "inX_addr.h"
#include "dataset_opt.h"

#define MAX_QNAME_SZ 512

typedef struct {
    struct timeval ts;
    inX_addr src_ip_addr;
    inX_addr dst_ip_addr;
    unsigned short src_port;
    unsigned short dst_port;
    unsigned char ip_version;
    unsigned char proto;
} transport_message;

typedef struct _dns_message dns_message;
struct _dns_message {
    transport_message *tm;
    inX_addr client_ip_addr;
    unsigned short qtype;
    unsigned short qclass;
    unsigned short msglen;
    char qname[MAX_QNAME_SZ];
    const char *tld;
    unsigned char opcode;
    unsigned char rcode;
    unsigned int malformed:1;
    unsigned int qr:1;
    unsigned int rd:1;		/* set if RECUSION DESIRED bit is set */
    unsigned int aa:1;		/* set if AUTHORITATIVE ANSWER bit is set */
    unsigned int tc:1;		/* set if TRUNCATED RESPONSE bit is set */
    struct {
	unsigned int found:1;	/* set if we found an OPT RR */
	unsigned int DO:1;	/* set if DNSSEC DO bit is set */
	unsigned char version;	/* version field from OPT RR */
	unsigned short bufsiz;	/* class field from OPT RR */
    } edns;
    /* ... */
};

typedef void (DMC) (dns_message *);

void dns_message_report(FILE *);
int dns_message_add_array(const char *, const char *,const char *,const char *,const char *,const char *, dataset_opt);
const char * dns_message_QnameToNld(const char *, int);
const char * dns_message_tld(dns_message * m);
void dns_message_init(void);
void dns_message_clear_arrays(void);

#ifndef T_OPT
#define T_OPT 41	/* OPT pseudo-RR, RFC2761 */
#endif

#ifndef T_AAAA
#define T_AAAA 28
#endif

#ifndef C_CHAOS
#define C_CHAOS 3
#endif
