#ifndef __dsc_qname_index_h
#define __dsc_qname_index_h

int qname_indexer(const void *);
int qname_iterator(char **label);
void qname_reset(void);
int second_ld_indexer(const void *);
int second_ld_iterator(char **label);
void second_ld_reset(void);
int third_ld_indexer(const void *);
int third_ld_iterator(char **label);
void third_ld_reset(void);

#endif /* __dsc_qname_index_h */
