/* Hapy is a public domain software. See Hapy README file for the details. */


#include <Hapy/Assert.h>
#include <Hapy/IoStream.h>

#include <cstring>
#include <errno.h>


void Hapy::Complain(const char *fname, int lineno) {
	cerr << fname << ':' << lineno << ": " << strerror(errno) << endl;
}


void Hapy::Abort(const char *fname, int lineno, const char *cond) {
	cerr << fname << ':' << lineno << ": assertion failed: '" 
		<< (cond ? cond : "?") << "'" << endl;
	::abort();
}

void Hapy::Exit(const char *fname, int lineno, const char *cond) {
	cerr << fname << ':' << lineno << ": assertion failed: '" 
		<< (cond ? cond : "?") << "'" << endl;
	::exit(-2);
}

void Hapy::Exit() {
	if (errno)
		::exit(errno);
	else
		::exit(-2);
}
