/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Algorithm.h>
#include <Hapy/IoStream.h>
#include <Hapy/RuleBase.h>

bool Hapy::Algorithm::isA(const string &) const {
	return false;
}

Hapy::ostream &Hapy::Algorithm::print(ostream &os) const {
	Should(false);
	os << "rule algorithm";
	return os;
}

bool Hapy::Algorithm::compileSubRule(RulePtr &r, const RulePtr &trimmer) {
	// fork in case other users have different preferences
	RulePtr old = r;
	r = RulePtr(new RuleBase(*old)); // leaking old pointers?
	return r->compile(trimmer);
}
