 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <climits>
#include <cctype>
#include <math.h>

#include "xstd/Math.h"


// try "ceil(700/0.7)" to see why xceil is needed
// also useful as "xceil(m1*m2, 1)" because "ceil(10*~0.9)" == 10 on Linux
double xceil(double nom, double denom) {
#if HAVE_CEILF
	return (double)ceilf((float)(nom/denom));
#else
	const double cex = ceil(nom/denom);
	const double cm1 = cex-1;
	const double cp1 = cex+1;

	const double dm1 = Abs(nom - cm1*denom);
	const double dex = Abs(nom - cex*denom);
	const double dp1 = Abs(nom - cp1*denom);

	if (dm1 <= dex && nom <= cm1*denom )
		return dm1 <= dp1 || !(nom <= cp1*denom) ? cm1 : cp1;
	else
		return dex <= dp1 || !(nom <= cp1*denom) ? cex : cp1;
#endif
}

int XGCD(int a, int b) {
	return b ? XGCD(b, a % b) : a;
}

double rint(double x) {
	return (int)(x + 0.5);
}
