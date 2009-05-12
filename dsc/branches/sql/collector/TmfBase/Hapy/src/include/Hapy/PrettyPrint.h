/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PRETTY_PRINT__H
#define HAPY_PRETTY_PRINT__H

#include <Hapy/String.h>
#include <Hapy/IosFwd.h>

namespace Hapy {

// prints [0, bufSize) section of a raw buffer,
// escaping non-printable characters and
// showing at most maxShow buffer characters
void PrettyPrint(ostream &os, const char *buffer, string::size_type bufSize, string::size_type maxShow = string::npos);

} // namespace

#endif

