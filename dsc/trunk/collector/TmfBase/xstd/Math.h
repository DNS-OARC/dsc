 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_MATH__H
#define TMF_BASE__XSTD_MATH__H

#include <math.h>

// abs, min, and max
template <class T> inline T Abs(const T &a) { return a > 0 ? a : -a; }
template <class T> inline T Min(const T &a, const T &b) { return (a <= b) ? a : b; }
template <class T> inline T Max(const T &a, const T &b) { return (a >= b) ? a : b; }
template <class T> inline T Min(const T &a, const T &b, const T &c) { return Min(Min(a,b), c); }
template <class T> inline T Max(const T &a, const T &b, const T &c) { return Max(Max(a,b), c); }
template <class T> inline T MiniMax(const T &l, const T &v, const T &h) { return Min(Max(l,v), h); }

// safe division
inline double Ratio(int n, double d) { return d ? (n ? n/d : 0) : -1; }
inline double Ratio(double n, double d) { return d ? n/d : -1; }
inline double Percent(int n, double d) { return Ratio(n, d/100); }
inline double Percent(double n, double d) { return Ratio(n, d/100); }

// try "ceil(700/0.7)" to see why xceil is needed
extern double xceil(double nom, double denom);

// the greatest common divider
extern int XGCD(int a, int b);

// to avoid problems with signed chars as indices into arrays
inline int xord(char c) { return (int)(unsigned char)c; }

#ifndef HAVE_RINT
	extern double rint(double x);
#endif

#endif

