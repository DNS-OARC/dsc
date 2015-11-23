#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dns_message.h"
#include "md_array.h"
#include "pcap.h"
#include "base64.h"
#include "xmalloc.h"

static const char *d1_type_s;	/* XXX barf */
static const char *d2_type_s;	/* XXX barf */

static const char *b64 = " base64=\"1\"";

static void
start_array(void *pr_data, const char *name)
{
    FILE *fp = pr_data;
    assert(fp);
    fprintf(fp, "<array");
    fprintf(fp, " name=\"%s\"", name);
    fprintf(fp, " dimensions=\"%d\"", 2);
    fprintf(fp, " start_time=\"%d\"", Pcap_start_time());
    fprintf(fp, " stop_time=\"%d\"", Pcap_finish_time());
    fprintf(fp, ">\n");
}

static void
finish_array(void *pr_data)
{
    FILE *fp = pr_data;
    fprintf(fp, "</array>\n");
}

static void
d1_type(void *pr_data, const char *t)
{
    FILE *fp = pr_data;
    fprintf(fp, "  <dimension number=\"1\" type=\"%s\"/>\n", t);
    d1_type_s = t;
}

static void
d2_type(void *pr_data, const char *t)
{
    FILE *fp = pr_data;
    fprintf(fp, "  <dimension number=\"2\" type=\"%s\"/>\n", t);
    d2_type_s = t;
}

static const char *entity_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
"0123456789._-:";

static void
d1_begin(void *pr_data, char *l)
{
    FILE *fp = pr_data;
    int ll = strlen(l);
    char *e = NULL;
    if (strspn(l, entity_chars) != ll) {
	int x = base64_encode(l, ll, &e);
	assert(x);
	l = e;
    }
    fprintf(fp, "    <%s val=\"%s\"%s>\n", d1_type_s, l, e ? b64 : "");
    if (e)
	xfree(e);
}

static void
print_element(void *pr_data, char *l, int val)
{
    FILE *fp = pr_data;
    int ll = strlen(l);
    char *e = NULL;
    if (strspn(l, entity_chars) != ll) {
	int x = base64_encode(l, ll, &e);
	assert(x);
	l = e;
    }
    fprintf(fp, "      <%s", d2_type_s);
    fprintf(fp, " val=\"%s\"%s", l, e ? b64 : "");
    fprintf(fp, " count=\"%d\"", val);
    fprintf(fp, "/>\n");
    if (e)
	xfree(e);
}

static void
d1_end(void *pr_data, char *l)
{
    FILE *fp = pr_data;
    fprintf(fp, "    </%s>\n", d1_type_s);
}

static void
start_data(void *pr_data)
{
    FILE *fp = pr_data;
    fprintf(fp, "  <data>\n");
}

static void
finish_data(void *pr_data)
{
    FILE *fp = pr_data;
    fprintf(fp, "  </data>\n");
}

md_array_printer xml_printer =
{
    start_array,
    finish_array,
    d1_type,
    d2_type,
    start_data,
    finish_data,
    d1_begin,
    d1_end,
    print_element
};
