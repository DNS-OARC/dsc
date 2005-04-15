#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_QNAME_SZ 512

typedef struct _dns_message dns_message;
struct _dns_message {
    struct timeval ts;
    struct in_addr client_ipv4_addr;
    unsigned short src_port;
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
    struct {
	unsigned int found:1;	/* set if we found an OPT RR */
	unsigned int DO:1;	/* set if DNSSEC DO bit is set */
	unsigned char version;	/* version field from OPT RR */
    } edns;
    /* ... */
};

typedef void (DMC) (dns_message *);

void dns_message_report(void);
int dns_message_add_array(const char *, const char *,const char *,const char *,const char *,const char *, int, int);
const char * dns_message_tld(dns_message * m);
void dns_message_init(void);

#ifndef T_OPT
#define T_OPT 41	/* OPT pseudo-RR, RFC2761 */
#endif
