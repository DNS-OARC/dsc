 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include "xstd/Assert.h"
#include "xstd/Math.h"
#include "Hapy/Buffer.h"
#include "Hapy/Parser.h"

using namespace Hapy;

Hapy::Buffer::Buffer(): thePos(0), theMaxProgress(0), isAtEnd(false) {
}

Hapy::Buffer::Buffer(const string &aData): theData(aData),
	thePos(0), theMaxProgress(0), isAtEnd(false) {
}

void Hapy::Buffer::append(const string &moreData) {
	theData += moreData;
}

void Hapy::Buffer::advance(string::size_type delta) {
	thePos += delta;
	if (!Should(thePos <= theData.size()))
		thePos = theData.size();
	theMaxProgress = Max(theMaxProgress, pos());
//cerr << here << "buffer advanced to " << pos() << " : " << content() << endl;
}

void Hapy::Buffer::backtrack(string::size_type delta) {
//cerr << here << "buffer (" << theProgress.size() << ") backtracking from " << pos();
	if (Should(thePos >= delta))
		thePos -= delta;
	else
		thePos = 0;
//cerr << " to " << pos() << " : " << content() << endl;
}

string::size_type Hapy::Buffer::pos() const {
	return thePos;
}

