/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_OPTIMIZER__H
#define HAPY_OPTIMIZER__H

#include <Hapy/Top.h>

namespace Hapy {

	// warning: this internal interface changes; use Hapy::Optimize() instead

	namespace Optimizer {

		// do we need finer control? time versus space optimizations?
		// this interface is meant to simplify adding more knobs

		extern bool IsEnabled; // do not read this directly

		// should we optimize?
		inline bool On() { return IsEnabled; }

	}
}

#endif
