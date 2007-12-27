/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_STRING__H
#define HAPY_STRING__H

#include <Hapy/config.h>
#include <string>

namespace Hapy {

using std::string;

// libstdc++ version 2 has a non-standard string::compare profile
inline
int string_compare(const string &s1, string::size_type pos1, string::size_type n1, const string &s2) {
#	if HAVE_SPN_STRING_COMPARE
		return s1.compare(s2, pos1, n1);  // string, position, number
#	else
		return s1.compare(pos1, n1, s2);  // position, number, string
#	endif
}

} // namespace

#endif
