#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <syslog.h>

#include "ip_message.h"
#include "md_array.h"

#include "ip_direction_index.h"
#include "ip_proto_index.h"

extern md_array_printer xml_printer;
static md_array_list *Arrays = NULL;

void
ip_message_handle(const struct ip *ip)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)  
        md_array_count(a->theArray, ip);
}

static int
ip_message_find_indexer(const char *in, IDXR ** ix, HITR ** it)
{
    if (0 == strcmp(in, "ip_direction")) {
        *ix = ip_direction_indexer;
        *it = ip_direction_iterator;
        return 1;
    }
    if (0 == strcmp(in, "ip_proto")) { 
        *ix = ip_proto_indexer;
        *it = ip_proto_iterator;
        return 1;
    }
    syslog(LOG_ERR, "unknown indexer '%s'", in);
    return 0;
}
    
static int  
ip_message_find_filter(const char *fn, FLTR ** f)
{
    if (0 == strcmp(fn, "any")) {
        *f = NULL;
        return 1;
    }
    syslog(LOG_ERR, "unknown filter '%s'", fn);
    return 0;
}
        
int
ip_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f)
{
    FLTR *filter;
    IDXR *indexer1;
    HITR *iterator1;
    IDXR *indexer2;
    HITR *iterator2;
    md_array_list *a;
 
    if (0 == ip_message_find_indexer(fi, &indexer1, &iterator1))
        return 0;
    if (0 == ip_message_find_indexer(si, &indexer2, &iterator2))
        return 0;
    if (0 == ip_message_find_filter(f, &filter))
        return 0;
        
    a = calloc(1, sizeof(*a));
    a->theArray = md_array_create(name, filter,
        fn, indexer1, iterator1, 
        sn, indexer2, iterator2);
    assert(a->theArray);
    a->next = Arrays;
    Arrays = a;
    return 1;
}       

void
ip_message_report(void)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
        md_array_print(a->theArray, &xml_printer);
}
