 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include "xstd/Clock.h"


Clock TheClock;

Clock::Clock() {
	update();
}

void Clock::update() { 
	const Time newTime = Time::Now();

	// gettimeofday(2) on FreeBSD-3.3 is buggy
	// make sure clocks never go backwards
	if (!theCurTime.known() || (newTime > theCurTime && newTime.tv_usec >= 0))
		theCurTime = newTime;

	if (!theStart.known())
		theStart = theCurTime;
}
