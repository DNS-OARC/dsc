 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "xstd/Time.h"
#include "xstd/UniqueNum.h"


long int UniqueNum() {
	static bool initialized = false;
	if (!initialized) {
#		if defined(HAVE_SRANDOMDEV)
			srandomdev();
#		else
			srandom(getpid() ^ Time::Now().usec());
#		endif
		initialized = true;
	}
	return random();
}
