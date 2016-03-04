#include "md_array.h"

void Pcap_init(const char *device, int promisc);
int Pcap_run();
void Pcap_close(void);
int Pcap_start_time(void);
int Pcap_finish_time(void);
void Pcap_start_time_iso8601(char *, int);
void Pcap_finish_time_iso8601(char *, int);
void pcap_report(FILE *, md_array_printer*);
