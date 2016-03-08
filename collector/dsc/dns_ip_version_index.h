#ifndef __dsc_dns_ip_version_index_h
#define __dsc_dns_ip_version_index_h

int dns_ip_version_indexer(const void *);
int dns_ip_version_iterator(char **label);
void dns_ip_version_reset(void);

#endif /* __dsc_dns_ip_version_index_h */
