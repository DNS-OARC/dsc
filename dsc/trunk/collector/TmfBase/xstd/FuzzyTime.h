 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_FUZZY_TIME__H
#define TMF_BASE__XSTD_FUZZY_TIME__H

#include "xstd/Time.h"
#include "xstd/Math.h"

class ostream;

// a [ beg, end ] Time approximation/estimate
class FuzzyTime {
	public:
		FuzzyTime() {}
		FuzzyTime(Time p): theBeg(p), theEnd(p) {}
		FuzzyTime(Time b, Time e): theBeg(b), theEnd(e) {}

		bool known() const { return theBeg.known() && theEnd.known() && theBeg <= theEnd; }

		bool includes(const Time &t) const { return theBeg <= t && t <= theEnd; }

		FuzzyTime &operator +=(const FuzzyTime &tm);
		FuzzyTime &operator -=(const FuzzyTime &tm);

		const Time &beg() const { return theBeg; }
		const Time &end() const { return theEnd; }

		ostream &print(ostream &os) const;

	private:
		Time theBeg;
		Time theEnd;
};

inline
FuzzyTime operator +(FuzzyTime t1, FuzzyTime t2) {
	return t1 += t2;
}

inline
FuzzyTime operator -(FuzzyTime t1, FuzzyTime t2) {
	return t1 -= t2;
}

inline
FuzzyTime Max(const FuzzyTime &t1, const FuzzyTime &t2) {
	return FuzzyTime(Max(t1.beg(), t2.beg()), Max(t1.end(), t2.end()));
}

inline
FuzzyTime Min(const FuzzyTime &t1, const FuzzyTime &t2) {
	return FuzzyTime(Min(t1.beg(), t2.beg()), Min(t1.end(), t2.end()));
}

inline
ostream &operator <<(ostream &os, const FuzzyTime &tm) {
	return tm.print(os);
}

#endif
