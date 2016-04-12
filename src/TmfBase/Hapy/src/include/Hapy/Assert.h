/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_ASSERT__H
#define HAPY_ASSERT__H

#include <Hapy/Top.h>

// our version of assert(3), mostly for portability purposes
#ifndef Assert
#define Assert(cond) ((cond) ? (void)0 : Hapy::Abort(__FILE__, __LINE__, #cond))
#endif

// same as Assert but calls Exit instead of Abort
#ifndef Check
#define Check(cond) ((cond) ? (void)0 : Hapy::Exit(__FILE__, __LINE__, #cond))
#endif

// logs current error to cerr and exits if cond fails
#ifndef Must
#define Must(cond) ((cond) || Hapy::Complain(__FILE__, __LINE__) || Hapy::Exit())
#endif

// logs current error to cerr if cond fails
#ifndef Should
#define Should(cond) ((cond) || Hapy::Complain(__FILE__, __LINE__))
#endif


// handy for temporary debugging
#ifndef here
#define here __FILE__ << ':' << __LINE__ << ": "
#endif

/* internal functions used by macros above */

namespace Hapy {

// logs current err to cerr
extern bool Complain(const char *fname, int lineno);

// aborts program execution and generates coredump
extern void Abort(const char *fname, int lineno, const char *cond);

// aborts program execution without coredump
extern bool Exit(const char *fname, int lineno, const char *cond);
extern bool Exit();

} // namespace

#endif
