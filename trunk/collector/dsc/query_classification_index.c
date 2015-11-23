#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/nameser.h>

/* OpenBSD does not have nameser_compat.h */
#ifdef __OpenBSD__
#define C_NONE          254
#else
#include <arpa/nameser_compat.h> 
#endif

#ifndef T_A6
#define T_A6 38
#endif

#include "dns_message.h"
#include "md_array.h"

enum {
    CLASS_OK,
    CLASS_NONAUTH_TLD,
    CLASS_ROOT_SERVERS_NET,
    CLASS_LOCALHOST,
    CLASS_A_FOR_A,
    CLASS_A_FOR_ROOT,
    CLASS_RFC1918_PTR,
    CLASS_FUNNY_QCLASS,
    CLASS_FUNNY_QTYPE,
    CLASS_SRC_PORT_ZERO,
    CLASS_MALFORMED
};

static const char *KnownTLDS[];

static int
nonauth_tld(const dns_message * m)
{
    unsigned int i;
    const char *tld = dns_message_tld((dns_message *) m);
    for (i = 0; KnownTLDS[i]; i++)
	if (0 == strcmp(KnownTLDS[i], tld))
	    return 0;
    return CLASS_NONAUTH_TLD;
}

static int
root_servers_net(const dns_message * m)
{
    if (0 == strcmp(m->qname + strlen(m->qname) - 16, "root-servers.net"))
	return CLASS_ROOT_SERVERS_NET;
    return 0;
}

static int
localhost(const dns_message * m)
{
    if (0 == strcmp(m->qname + strlen(m->qname) - 9, "localhost"))
	return CLASS_LOCALHOST;
    return 0;
}

static int
a_for_a(const dns_message * m)
{
    struct in_addr a;
    if (m->qtype != T_A)
	return 0;
    if (inet_aton(m->qname, &a))
	return CLASS_A_FOR_A;
    return 0;
}

static int
a_for_root(const dns_message * m)
{
    if (m->qtype != T_A)
	return 0;
    if (0 == strcmp(m->qname, "."))
	return CLASS_A_FOR_ROOT;
    return 0;
}

static int
rfc1918_ptr(const dns_message * m)
{
    char *t;
    char q[128];
    unsigned int i = 0;
    if (m->qtype != T_PTR)
	return 0;
    strncpy(q, m->qname, sizeof(q) - 1);
    q[sizeof(q) - 1] = '\0';
    if (NULL == (t = strstr(q, ".in-addr.arpa")))
	return 0;
    *t = '\0';
    for (t = strtok(q, "."); t; t = strtok(NULL, ".")) {
	i >>= 8;
	i |= ((atoi(t) & 0xff) << 24);
    }
    if ((i & 0xff000000) == 0x0a000000)		/* 10.0.0.0/8 */
	return CLASS_RFC1918_PTR;
    if ((i & 0xfff00000) == 0xac100000)		/* 172.16.0.0/12 */
	return CLASS_RFC1918_PTR;
    if ((i & 0xffff0000) == 0xc0a80000)		/* 192.168.0.0/16 */
	return CLASS_RFC1918_PTR;
    return 0;
}

static int
funny_qclass(const dns_message * m)
{
    switch (m->qclass) {
    case C_IN:
    case C_CHAOS:
    case C_ANY:
    case C_NONE:
    case C_HS:
	return 0;
	break;
    default:
	return CLASS_FUNNY_QCLASS;
	break;
    }
    return CLASS_FUNNY_QCLASS;
}

static int
funny_qtype(const dns_message * m)
{
    switch (m->qclass) {
    case T_A:
    case T_NS:
    case T_MD:
    case T_MF:
    case T_CNAME:
    case T_SOA:
    case T_MB:
    case T_MG:
    case T_MR:
    case T_NULL:
    case T_WKS:
    case T_PTR:
    case T_HINFO:
    case T_MINFO:
    case T_MX:
    case T_TXT:
    case T_RP:
    case T_AFSDB:
    case T_X25:
    case T_ISDN:
    case T_RT:
    case T_NSAP:
    case T_NSAP_PTR:
    case T_SIG:
    case T_KEY:
    case T_PX:
    case T_GPOS:
    case T_AAAA:
    case T_A6:
    case T_LOC:
    case T_NXT:
    case T_EID:
    case T_NIMLOC:
    case T_SRV:
    case T_ATMA:
    case T_NAPTR:
    case T_OPT:
    case T_IXFR:
    case T_AXFR:
    case T_MAILB:
    case T_MAILA:
    case T_ANY:
	return 0;
	break;
    default:
	return CLASS_FUNNY_QTYPE;
	break;
    }
    return CLASS_FUNNY_QTYPE;
}

static int
src_port_zero(const dns_message * m)
{
    if (0 == m->tm->src_port)
	return CLASS_SRC_PORT_ZERO;
    return 0;
}

static int
malformed(const dns_message * m)
{
    if (m->malformed)
	return CLASS_MALFORMED;
    return 0;
}

int
query_classification_indexer(const void *vp)
{
    const dns_message *m = vp;
    int x;
    if ((x = malformed(m)))
	return x;
    if ((x = src_port_zero(m)))
	return x;
    if ((x = funny_qclass(m)))
	return x;
    if ((x = funny_qtype(m)))
	return x;
    if ((x = a_for_a(m)))
	return x;
    if ((x = a_for_root(m)))
	return x;
    if ((x = localhost(m)))
	return x;
    if ((x = root_servers_net(m)))
	return x;
    if ((x = nonauth_tld(m)))
	return x;
    if ((x = rfc1918_ptr(m)))
	return x;
    return CLASS_OK;
}

int
query_classification_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = 0;
	return CLASS_MALFORMED + 1;
    }
    if (CLASS_OK == next_iter)
	*label = "ok";
    else if (CLASS_NONAUTH_TLD == next_iter)
	*label = "non-auth-tld";
    else if (CLASS_ROOT_SERVERS_NET == next_iter)
	*label = "root-servers.net";
    else if (CLASS_LOCALHOST == next_iter)
	*label = "localhost";
    else if (CLASS_A_FOR_ROOT == next_iter)
	*label = "a-for-root";
    else if (CLASS_A_FOR_A == next_iter)
	*label = "a-for-a";
    else if (CLASS_RFC1918_PTR == next_iter)
	*label = "rfc1918-ptr";
    else if (CLASS_FUNNY_QCLASS == next_iter)
	*label = "funny-qclass";
    else if (CLASS_FUNNY_QTYPE == next_iter)
	*label = "funny-qtype";
    else if (CLASS_SRC_PORT_ZERO == next_iter)
	*label = "src-port-zero";
    else if (CLASS_MALFORMED == next_iter)
	*label = "malformed";
    else
	return -1;
    return next_iter++;
}

static const char *KnownTLDS[] =
{
    "com",
    "net",
    "arpa",
    "org",
    "edu",
    "aero",
    "biz",
    "coop",
    "info",
    "museum",
    "name",
    "pro",
    "gov",
    "mil",
    "int",
    ".",
    "ac",
    "ad",
    "ae",
    "af",
    "ag",
    "ai",
    "al",
    "am",
    "an",
    "ao",
    "aq",
    "ar",
    "as",
    "at",
    "au",
    "aw",
    "az",
    "ba",
    "bb",
    "bd",
    "be",
    "bf",
    "bg",
    "bh",
    "bi",
    "bj",
    "bm",
    "bn",
    "bo",
    "br",
    "bs",
    "bt",
    "bv",
    "bw",
    "by",
    "bz",
    "ca",
    "cc",
    "cd",
    "cf",
    "cg",
    "ch",
    "ci",
    "ck",
    "cl",
    "cm",
    "cn",
    "co",
    "cr",
    "cu",
    "cv",
    "cx",
    "cy",
    "cz",
    "de",
    "dj",
    "dk",
    "dm",
    "do",
    "dz",
    "ec",
    "ee",
    "eg",
    "eh",
    "er",
    "es",
    "et",
    "fi",
    "fj",
    "fk",
    "fm",
    "fo",
    "fr",
    "ga",
    "gd",
    "ge",
    "gf",
    "gg",
    "gh",
    "gi",
    "gl",
    "gm",
    "gn",
    "gp",
    "gq",
    "gr",
    "gs",
    "gt",
    "gu",
    "gw",
    "gy",
    "hk",
    "hm",
    "hn",
    "hr",
    "ht",
    "hu",
    "id",
    "ie",
    "il",
    "im",
    "in",
    "io",
    "iq",
    "ir",
    "is",
    "it",
    "je",
    "jm",
    "jo",
    "jp",
    "ke",
    "kg",
    "kh",
    "ki",
    "km",
    "kn",
    "kp",
    "kr",
    "kw",
    "ky",
    "kz",
    "la",
    "lb",
    "lc",
    "li",
    "lk",
    "lr",
    "ls",
    "lt",
    "lu",
    "lv",
    "ly",
    "ma",
    "mc",
    "md",
    "mg",
    "mh",
    "mk",
    "ml",
    "mm",
    "mn",
    "mo",
    "mp",
    "mq",
    "mr",
    "ms",
    "mt",
    "mu",
    "mv",
    "mw",
    "mx",
    "my",
    "mz",
    "na",
    "nc",
    "ne",
    "nf",
    "ng",
    "ni",
    "nl",
    "no",
    "np",
    "nr",
    "nu",
    "nz",
    "om",
    "pa",
    "pe",
    "pf",
    "pg",
    "ph",
    "pk",
    "pl",
    "pm",
    "pn",
    "pr",
    "ps",
    "pt",
    "pw",
    "py",
    "qa",
    "re",
    "ro",
    "ru",
    "rw",
    "sa",
    "sb",
    "sc",
    "sd",
    "se",
    "sg",
    "sh",
    "si",
    "sj",
    "sk",
    "sl",
    "sm",
    "sn",
    "so",
    "sr",
    "st",
    "sv",
    "sy",
    "sz",
    "tc",
    "td",
    "tf",
    "tg",
    "th",
    "tj",
    "tk",
    "tm",
    "tn",
    "to",
    "tp",
    "tr",
    "tt",
    "tv",
    "tw",
    "tz",
    "ua",
    "ug",
    "uk",
    "um",
    "us",
    "uy",
    "uz",
    "va",
    "vc",
    "ve",
    "vg",
    "vi",
    "vn",
    "vu",
    "wf",
    "ws",
    "ye",
    "yt",
    "yu",
    "za",
    "zm",
    "zw",
    NULL
};
