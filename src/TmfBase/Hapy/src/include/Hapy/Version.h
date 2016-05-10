/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_VERSION__H
#define HAPY_VERSION__H

#include <Hapy/Top.h>

namespace Hapy {

// this internal representation may change
typedef unsigned long int Version;

// returns version suitable for comparison
inline Version version() {
	typedef unsigned long int Component;
	const Component format = 0U;
	const Component major = 0U;
	const Component minor = 0U;
	const Component plevel = 6U;
	const Component special = 0U;

	return
		(format << 30) +
		(major << 24) +
		(minor << 16) +
		(plevel << 8) +
		special;
}

} // namespace

#endif
