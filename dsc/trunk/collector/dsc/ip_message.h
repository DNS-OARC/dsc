
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

typedef void (IPC) (const struct ip *);
IPC ip_message_handle;

void ip_message_report(void);
