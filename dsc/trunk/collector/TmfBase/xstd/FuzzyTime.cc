 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <iostream>

#include "xstd/FuzzyTime.h"


FuzzyTime &FuzzyTime::operator +=(const FuzzyTime &tm) {
	theBeg += tm.theBeg;
	theEnd += tm.theEnd;
	return *this;
}

FuzzyTime &FuzzyTime::operator -=(const FuzzyTime &tm) {
	if (theBeg > tm.theEnd)
		theBeg -= tm.theEnd;
	else
		theBeg = Time(0,0);

	if (theEnd > tm.theBeg)
		theEnd -= tm.theBeg;
	else
		theEnd = theBeg = Time(0,0);

	return *this;
}

ostream &FuzzyTime::print(ostream &os) const {
	if (!known())
		return os << "unknown";
	else
	if (theBeg == theEnd)
		return os << '~' << theBeg;
	else
		return os << "between " << theBeg << " and " << theEnd;
}
