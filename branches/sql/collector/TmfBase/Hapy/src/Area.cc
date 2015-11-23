/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Area.h>
#include <Hapy/IoStream.h>
#include <Hapy/PrettyPrint.h>


Hapy::Area::Area():	theStart(0), theSize(0), theState(asNone) {
	// clear();
}

void Hapy::Area::clear() {
	theStart = 0;
	theSize = 0;
	theState = asNone;
	theImage = string();
}

const char *Hapy::Area::imageData() const {
	return theState == asFrozen ?
		theImage.data() :
		(theImage.data() + start());
}

Hapy::Area::size_type Hapy::Area::imageSize() const {
	return theState == asFrozen ? theImage.size() : theSize;
}

bool Hapy::Area::operator ==(const Area &a) const {
	// we do not compare images
	return theStart == a.theStart && size() == a.size();
}

bool Hapy::Area::operator !=(const Area &a) const {
	return !(*this == a);
}

const Hapy::string &Hapy::Area::image() const {
	if (theState != asFrozen) {
		theImage = theImage.substr(start(), size()); // expensive
		theState = asFrozen;
	}
	return theImage;
}

void Hapy::Area::image(const string &anImage) {
	theImage = anImage; // cheap
	theState = asFrozen;
}

Hapy::ostream &Hapy::operator <<(ostream &os, const Area &a) {
	const string::size_type maxSize = 45;
	PrettyPrint(os, a.imageData(), a.imageSize(), maxSize);
	return os;
}
