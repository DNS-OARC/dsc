 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_IOMANIP__H
#define TMF_BASE__XSTD_IOMANIP__H

#include <stdlib.h>
#include "xstd/iosfwd.h"

#if defined(HAVE_IOMANIP_H)
#include <iomanip.h>
#elif defined(HAVE_IOMANIP)
#include <iomanip>
#endif

// handy manipulators to terminate a program after an error message
inline ostream &xabort(ostream &os) { ::abort(); return os; }
inline ostream &xexit(ostream &os) { ::exit(-1); return os; }

#endif
