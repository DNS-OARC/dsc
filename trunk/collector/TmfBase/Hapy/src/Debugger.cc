/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Debugger.h>
#include <Hapy/String.h>
#include <cstdlib>

Hapy::Debugger::Level Hapy::Debugger::TheLevel = Hapy::Debugger::dbgDefault;


void Hapy::Debugger::Configure() {
	// enable debugging if the code writer did not explicitly prohibit it 
	// and the code runner set the HAPY_DEBUG environmental variable
	if (TheLevel == dbgDefault) {
		if (const char *debug = ::getenv("HAPY_DEBUG")) {
			if (string("NONE") == debug)
				Hapy::Debug(false);
			else
			if (string("USER") == debug)
				Hapy::Debug(true);
			else
			if (string("ALL") == debug)
				TheLevel = dbgAll;
			else
				Should(false);
		}
	}
}
