/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/SizeCalc.h>
#include <limits>

const Hapy::SizeCalcLocal::size_type Hapy::SizeCalcLocal::nsize =
	std::numeric_limits<Hapy::SizeCalcLocal::size_type>::max();

Hapy::SizeCalcPass::SizeCalcPass(void *anId): id(anId), depth(0),
	minLoopDepth(-1) {
}



Hapy::SizeCalcLocal::SizeCalcLocal(): theValue(0), theDonePass(0),
	theDepth(-1), isSet(false), isDoing(false) {
}

Hapy::SizeCalcLocal::size_type Hapy::SizeCalcLocal::value() const {
	Assert(withValue());
	return theValue;
}

void Hapy::SizeCalcLocal::value(size_type aValue) {
	if (withValue()) {
		Assert(theValue == aValue); // must not change
	} else {
		Assert(aValue >= 0);
		theValue = aValue;
		isSet = true;
	}
}

bool Hapy::SizeCalcLocal::doing(const SizeCalcPass &pass) const {
	return isDoing;
}

//bool Hapy::SizeCalcLocal::did(const SizeCalcPass &pass) const {
//	Assert(theDonePass <= pass.id);
//	return !isDoing && (theDonePass >= pass.id);
//}

void Hapy::SizeCalcLocal::start(SizeCalcPass &pass) {
	Assert(!doing(pass));
	theDepth = ++pass.depth;
	isDoing = true;
}

void Hapy::SizeCalcLocal::stop(SizeCalcPass &pass) {
	Assert(doing(pass));
	--pass.depth;
	isDoing = false;
	theDonePass = pass.id;
}
