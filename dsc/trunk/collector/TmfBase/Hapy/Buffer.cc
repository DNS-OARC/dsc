 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include "xstd/Assert.h"
#include "xstd/Math.h"
#include "Hapy/Buffer.h"
#include "Hapy/Parser.h"

using namespace Hapy;

Hapy::Buffer::Buffer(): theMaxProgress(0),
	theSkippedSize(0), isPrepped(false), isSkipPaused(false),
	isAtEnd(false) {
}

Hapy::Buffer::Buffer(const string &aData): theData(aData), theMaxProgress(0),
	theSkippedSize(0), isPrepped(false), isSkipPaused(false),
	isAtEnd(false) {
}

void Hapy::Buffer::skipper(Rule &aSkipper) {
	Assert(!theSkipper.hasAlg());
	theSkipper = aSkipper;
}

void Hapy::Buffer::pauseSkip() {
	Assert(!isSkipPaused);
	isSkipPaused = true;
}

void Hapy::Buffer::resumeSkip() {
	Assert(isSkipPaused);
	isSkipPaused = false;
}

void Hapy::Buffer::append(const string &moreData) {
	theData += moreData;
}

// advances skip position
void Hapy::Buffer::prep() {
	if (!isPrepped && theSkipper.hasAlg() && !isSkipPaused) {
		isPrepped = true;
		Assert(!theSkippedSize);

		Buffer b(content());
		PreeNode n;
		Result::StatusCode sc = theSkipper.firstMatch(b, n);
		if (sc == Result::scMatch)
			theSkippedSize = b.pos();
		else
			Should(sc == Result::scMiss); // XXX: handle scMore too
	}
}

void Hapy::Buffer::advance(string::size_type delta) {
	theProgress.push_back(pos() + delta);
	isPrepped = false;
	theSkippedSize = 0;
	theMaxProgress = Max(theMaxProgress, pos());
//cerr << here << "buffer advanced to " << pos() << " : " << content() << endl;
}

void Hapy::Buffer::backtrack() {
//cerr << here << "buffer (" << theProgress.size() << ") backtracking from " << pos();
	theProgress.pop_back();
	isPrepped = false;
	theSkippedSize = 0;
//cerr << " to " << pos() << " : " << content() << endl;
}

string::size_type Hapy::Buffer::pos() const {
	return theSkippedSize + 
		(theProgress.size() > 0 ? theProgress.back() : 0);
}

