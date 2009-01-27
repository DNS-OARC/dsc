/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_TOP__H
#define HAPY_TOP__H

// global settings affecting all Hapy code
// Hapy sources always include this file, and always include it first


#include <Hapy/config.h>


namespace Hapy {

	// user-level knobs to control debugging and optimization
	extern void Debug(bool doIt);
	extern void Optimize(bool doIt);

}

#endif
