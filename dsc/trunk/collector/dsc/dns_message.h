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
    unsigned short qtype;
    unsigned short qclass;
    char qname[MAX_QNAME_SZ];
    unsigned char rcode;
    unsigned int qr:1;
    /* ... */
};

typedef void (DMC) (dns_message *);

void dns_message_init(void);
void dns_message_report(void);
