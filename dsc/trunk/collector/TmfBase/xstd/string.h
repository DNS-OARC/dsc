 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_STRING__H
#define TMF_BASE__XSTD_STRING__H

#include "xstd/xstd.h"

#ifdef HAVE_STRING
#include <string>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#if !defined(HAVE_STRCASECMP) && defined(HAVE_STRICMP)
	inline
	int strcasecmp(const char *s1, const char *s2) {
		return stricmp(s1, s2);
	}
#endif

#if !defined(HAVE_STRNCASECMP) && defined(HAVE_STRNICMP)
	inline
	int strncasecmp(const char *s1, const char *s2, size_t sz) {
		return strnicmp(s1, s2, sz);
	}
#endif

#endif
