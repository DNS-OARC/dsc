 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_ASSERT__H
#define TMF_BASE__XSTD_ASSERT__H

#include <stdlib.h>

class ostream;

// our version of assert(3), mostly for portability purposes
#define Assert(cond) ((cond) ? (void)0 : Abort(__FILE__, __LINE__, #cond))

// same as Assert but calls Exit instead of Abort
#define Check(cond) ((cond) ? (void)0 : Exit(__FILE__, __LINE__, #cond))

// logs current error to cerr and exits if cond fails
#define Must(cond) ((cond) ? (void)0 : (Complain(__FILE__, __LINE__), Exit()))

// logs current error to cerr if cond fails
#define Should(cond) ((cond) ? true : (Complain(__FILE__, __LINE__), false))


// handy for temporary debugging
#define here __FILE__ << ':' << __LINE__ << ": "

/* internal functions used by macros above */

// logs current err to cerr
extern void Complain(const char *fname, int lineno);

// aborts program execution and generates coredump
extern void Abort(const char *fname, int lineno, const char *cond);

// aborts program execution without coredump
extern void Exit(const char *fname, int lineno, const char *cond);
extern void Exit();

#endif
