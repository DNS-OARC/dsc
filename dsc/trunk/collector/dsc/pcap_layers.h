#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

typedef int l7_callback(const u_char *, int , void *);

extern int (*callback_ether) (const u_char * pkt, int len, void *userdata);
extern int (*callback_vlan) (unsigned short vlan, void *userdata);
extern int (*callback_ipv4) (const struct ip *ipv4, int len, void *userdata);
extern int (*callback_ipv6) (const struct ip6_hdr *ipv6, int len, void *userdata);
extern int (*callback_gre) (const u_char *pkt, int len, void *userdata);
extern int (*callback_tcp) (const struct tcphdr *tcp, int len, void *userdata);
extern int (*callback_udp) (const struct udphdr *udp, int len, void *userdata);
extern int (*callback_tcp_sess) (const struct tcphdr *tcp, int len, void *userdata, l7_callback *);
extern int (*callback_l7) (const u_char * l7, int len, void *userdata);

extern void handle_pcap(u_char * userdata, const struct pcap_pkthdr *hdr, const u_char * pkt);
extern int pcap_layers_init(int dlt, int reassemble);
