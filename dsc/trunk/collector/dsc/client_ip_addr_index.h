int cip_indexer(const void *);
int cip_iterator(char **label);
void cip_reset(void);

/* shared between client_ip_addr_index and server_ip_addr_index */
unsigned int ipaddr_hashfunc(const void *key);
int ipaddr_cmpfunc(const void *a, const void *b);
