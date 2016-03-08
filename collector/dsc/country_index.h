#ifndef __dsc_country_index_h
#define __dsc_country_index_h


/* check HAVE_LIBGEOIP before #including this file */

int country_indexer(const void *);
int country_iterator(char **label);
void country_reset(void);

#endif /* __dsc_country_index_h */
