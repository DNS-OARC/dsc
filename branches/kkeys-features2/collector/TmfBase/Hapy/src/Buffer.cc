/* Hapy is a public domain software. See Hapy README file for the details. */


#include <Hapy/Assert.h>
#include <Hapy/Buffer.h>
#include <Hapy/Parser.h>
#include <Hapy/PrettyPrint.h>


// keep in sync with moveOn
Hapy::Buffer::Buffer(): thePos(0), theMaxProgress(0), didSeeEnd(false) {
}

// keep in sync with moveOn
Hapy::Buffer::Buffer(const string &aData): theData(aData),
	thePos(0), theMaxProgress(0), didSeeEnd(false) {
}

// keep in sync with ctors
void Hapy::Buffer::moveOn() {
	theData = empty() ? string() : content();
	thePos = 0;
	theMaxProgress = 0;
	// didSeeEnd is preserved
}

Hapy::string Hapy::Buffer::content(const size_type off) const {
	return theData.size() ? theData.substr(pos() + off) : theData;
}

bool Hapy::Buffer::startsWith(const string &s, const size_type off) const {
	return string_compare(theData, pos() + off, s.size(), s) == 0;
}

void Hapy::Buffer::append(const string &moreData) {
	theData += moreData;
}

void Hapy::Buffer::advance(string::size_type delta) {
	thePos += delta;
	if (!Should(thePos <= theData.size()))
		thePos = theData.size();
	if (theMaxProgress < pos())
		theMaxProgress = pos();
//cerr << here << "buffer advanced to " << pos() << " : " << content() << endl;
}

void Hapy::Buffer::backtrack(string::size_type delta) {
	backtrackTo(Should(thePos >= delta) ? thePos - delta : 0);
}

void Hapy::Buffer::backtrackTo(size_type pos) {
//cerr << here << "buffer (" << theProgress.size() << ") backtracking from " << pos();
	if (Should(pos <= thePos))
		thePos = pos;
//cerr << " to " << pos() << " : " << content() << endl;
}

Hapy::string::size_type Hapy::Buffer::pos() const {
	return thePos;
}

void Hapy::Buffer::print(ostream &os, size_type maxSize) const {
	PrettyPrint(os, theData.data() + pos(), contentSize(), maxSize);
}

