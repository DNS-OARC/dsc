 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_CLOCK__H
#define TMF_BASE__XSTD_CLOCK__H

#include "xstd/Time.h"

// current time maintainer (i.e., wall clock)
// it is assumed that only one clock, TheClock below, is needed
class Clock {
	public:
		Clock(); // calls update() and sets start time

		Time start() const { return theStart; }
		Time runTime() const { return time() - start(); }
	
		Time time() const { return theCurTime; }
		operator Time() const { return time(); }

		void update(); // updates current time using Time::Now()

	protected:
		Time theStart;   // approximate process start time
		Time theCurTime; // approximate current time
};

extern Clock TheClock;

#endif
