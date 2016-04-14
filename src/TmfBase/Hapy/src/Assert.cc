/* Hapy is a public domain software. See Hapy README file for the details. */


#include <Hapy/Assert.h>
#include <Hapy/IoStream.h>

#include <cstring>
#include <cstdlib>
#include <errno.h>


bool Hapy::Complain(const char *fname, int lineno) {
	cerr << fname << ':' << lineno << ": " << strerror(errno) << endl;
	return false;
}


void Hapy::Abort(const char *fname, int lineno, const char *cond) {
	cerr << fname << ':' << lineno << ": assertion failed: '" 
		<< (cond ? cond : "?") << "'" << endl;
	::abort();
}

bool Hapy::Exit(const char *fname, int lineno, const char *cond) {
	cerr << fname << ':' << lineno << ": assertion failed: '" 
		<< (cond ? cond : "?") << "'" << endl;
	::exit(-2);
	return false;
}

bool Hapy::Exit() {
	if (errno)
		::exit(errno);
	else
		::exit(-2);
	return false;
}
