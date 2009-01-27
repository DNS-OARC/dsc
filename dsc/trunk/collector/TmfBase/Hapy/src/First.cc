/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/First.h>


Hapy::First::First(): hasEmptySeq(false) {
}

void Hapy::First::clear() {
	theSet.clear();
	hasEmptySeq = false;
}

// union
Hapy::First &Hapy::First::operator +=(const First &f) {
	theSet.insert(f.theSet.begin(), f.theSet.end());
	hasEmptySeq = hasEmptySeq || f.hasEmptySeq;
	return *this;
}

// difference
Hapy::First &Hapy::First::operator -=(const First &f) {
	for (Set::const_iterator i = f.theSet.begin(); i != f.theSet.end(); ++i) {
		const Set::iterator pos = theSet.find(*i);
		if (pos != theSet.end())
			theSet.erase(pos);
	}
	hasEmptySeq = hasEmptySeq && !f.hasEmptySeq;
	return *this;
}

void Hapy::First::include(char c) {
	theSet.insert(c);
}

void Hapy::First::includeAny() {
	includeRange(0, 255);
}

void Hapy::First::includeRange(int first, int last) {
	for (int k = first; k <= last; ++k)
		include((Set::key_type)k);
}

void Hapy::First::includeEmptySequence(bool doIt) {
	hasEmptySeq = doIt;
}

