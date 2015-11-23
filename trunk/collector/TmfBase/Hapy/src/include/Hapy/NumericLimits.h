/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_NUMERIC_LIMITS__H
#define HAPY_NUMERIC_LIMITS__H

#include <Hapy/Top.h>

#ifdef HAPY_HAVE_LIMITS
#	include <limits>
#elif HAPY_HAVE_CLIMITS
#	include <climits>
#elif HAPY_HAVE_LIMITS_H
#	include <limits.h>
#endif


#ifndef HAPY_HAVE_NUMERIC_LIMITS
namespace std {
#	ifndef ULONG_MAX
#		define LONG_MIN INT_MIN
#		define LONG_MAX INT_MAX
#	endif

	template <class T>
	class numeric_limits {
		public:
			static T min() { return 0; }
			static T max() { return 0; }
	};

	signed long numeric_limits<signed long>::min() {
		return LONG_MIN;
	}

	signed long numeric_limits<signed long>::max() {
		return LONG_MAX;
	}
}
#endif

#endif
