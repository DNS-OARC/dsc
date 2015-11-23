/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_DEBUGGER__H
#define HAPY_DEBUGGER__H

#include <Hapy/Top.h>

namespace Hapy {

	// warning: this internal interface may change; use Hapy::Debug() instead

	namespace Debugger {

		typedef enum { dbgDefault, dbgNone, dbgUser, dbgAll } Level;
		extern Level TheLevel; // debugging detail level

		// should we debug at the given [code] level?
		inline bool On(Level level) { return TheLevel >= level; }

		extern void Configure(); // selects defaults based on environment

	}
}

#endif
