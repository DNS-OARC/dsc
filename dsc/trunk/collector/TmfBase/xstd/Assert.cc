 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <iostream>
#include <cstring>
#include <errno.h>

#include "xstd/Assert.h"


void Complain(const char *fname, int lineno) {
	cerr << fname << ':' << lineno << ": " << strerror(errno) << endl;
}


void Abort(const char *fname, int lineno, const char *cond) {
	cerr << fname << ':' << lineno << ": assertion failed: '" 
		<< (cond ? cond : "?") << "'" << endl;
	::abort();
}

void Exit(const char *fname, int lineno, const char *cond) {
	cerr << fname << ':' << lineno << ": assertion failed: '" 
		<< (cond ? cond : "?") << "'" << endl;
	::exit(-2);
}

void Exit() {
	if (errno)
		::exit(errno);
	else
		::exit(-2);
}
