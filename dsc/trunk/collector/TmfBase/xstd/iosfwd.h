 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_IOS_FWD__H
#define TMF_BASE__XSTD_IOS_FWD__H

#ifdef HAVE_IOSFWD_H
#	include <iosfwd.h>
#elif HAVE_IOSFWD
#	include <iosfwd>
#else
	class ostream;
	class istream;
#endif

#endif
