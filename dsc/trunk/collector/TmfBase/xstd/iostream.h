 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_IOSTREAM__H
#define TMF_BASE__XSTD_IOSTREAM__H

#include "xstd/iosfwd.h"

// we curerntly give priority to old, backword compatible headers
#ifdef HAVE_IOSTREAM_H
#	include <iostream.h>
#elif HAVE_IOSTREAM
#	include <ios>
#	include <iostream>
#endif

#ifdef HAVE_TYPE_IOS_FMTFLAGS
	typedef ios::fmtflags ios_fmtflags;
#elif HAVE_TYPE_IOS_BASE_FMTFLAGS
	typedef std::ios_base::fmtflags ios_fmtflags;
#else
	typedef long ios_fmtflags;
#endif

#endif
