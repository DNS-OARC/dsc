#include <stdio.h>
#include <assert.h>

#include "dns_message.h"
#include "md_array.h"

static FILE *fp;
static char *d1_type_s;		/* XXX barf */
static char *d2_type_s;		/* XXX barf */

static void
start_array(void)
{
    fp = fopen("out.xml", "w");
    assert(fp);
    fprintf(fp, "<array ");
    fprintf(fp, "dimensions=\"%d\"", 2);
    fprintf(fp, ">\n");
}

static void
finish_array(void)
{
    fprintf(fp, "</array>\n");
    if (stderr != fp)
	fclose(fp);
    fp = NULL;
}

static void
d1_type(char *t)
{
    fprintf(fp, "  <dimension number=\"1\" type=\"%s\">\n", t);
    d1_type_s = t;
}

static void
d2_type(char *t)
{
    fprintf(fp, "  <dimension number=\"2\" type=\"%s\">\n", t);
    d2_type_s = t;
}

static void
d1_begin(char *l)
{
    fprintf(fp, "    <%s=\"%s\">\n", d1_type_s, l);
}

static void
print_element(char *l, int val)
{
    fprintf(fp, "      <%s=\"%s\" count=\"%d\"/>\n", d2_type_s, l, val);
}

static void
d1_end(char *l)
{
    fprintf(fp, "    </%s>\n", d1_type_s);
}

static void
start_data(void)
{
    fprintf(fp, "  <data>\n");
}

static void
finish_data(void)
{
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
