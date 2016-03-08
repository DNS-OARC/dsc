#ifndef __dsc_dns_source_port_index_h
#define __dsc_dns_source_port_index_h

int dns_source_port_indexer(const void *);
int dns_source_port_iterator(char **label);
void dns_source_port_reset(void);

int dns_sport_range_indexer(const void *);
int dns_sport_range_iterator(char **label);
void dns_sport_range_reset(void);

#endif /* __dsc_dns_source_port_index_h */
