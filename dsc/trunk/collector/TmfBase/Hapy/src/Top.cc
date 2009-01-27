/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Debugger.h>
#include <Hapy/Optimizer.h>

void Hapy::Debug(bool doIt) {
	Debugger::TheLevel = doIt ? Debugger::dbgUser : Debugger::dbgNone;
}

void Hapy::Optimize(bool doIt) {
	Optimizer::IsEnabled = doIt;
}
