 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_STRSTREAM__H
#define TMF_BASE__XSTD_STRSTREAM__H

#include "xstd/xstd.h"

// newer sstream uses std::string and is not available in GCC 2.95.2
// we are trying to mimic sstream-like interface if it is not available

#if defined(HAVE_SSTREAM)
#   include <sstream>
#else
#	if defined(HAVE_STRSTREAM_H)
#		include <strstream.h>
#	elif defined(HAVE_STRSTREA_H) /* MS file name limit */
#		include <strstrea.h>
#	endif

#	include <string>

	class istringstream: public istrstream {
		public:
			istringstream(const string &buf):
				istrstream(buf.data(), buf.size()) {}
	};

	class ostringstream: public ostrstream {
		public:
			string str() const { 
				return string(((ostrstream*)this)->str()); }
	};

#endif

#ifdef HAVE_SSTREAMFREEZE
	template <class Stream>
	inline
	void streamFreeze(Stream &os, bool be) {
		os.freeze(be);
	}
#else
	template <class Stream>
	inline
	void streamFreeze(Stream &os, bool be) {
		os.rdbuf()->freeze(be);
	}
#endif


#endif
