 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <iostream>
#include <cstring>

#include "xstd/Error.h"


const Error Error::Last() {
#if defined(HAVE_GETLASTERROR)
	return Error(GetLastError());
#else
	return Error(errno);
#endif
}

const Error Error::Last(const Error &err) {
#if defined(HAVE_SETLASTERROR)
	SetLastError(err.no());
#else
	errno = err.no();
#endif
	return err;
}

const Error Error::LastExcept(const Error &exc) {
	const Error err = Last();
	return err == exc ? Error() : err;
}

const char *Error::str() const {
#if defined(FORMAT_MESSAGE_FROM_SYSTEM)
	static char buf[1024];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, no(), 0, buf, sizeof(buf), 0);
	return buf;
#else
	return strerror(no());
#endif
}


ostream &Error::print(ostream &os, const char *str) const {
	return os << '(' << no() << ") " << str;
}
