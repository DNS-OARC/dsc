 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include <functional>
#include <algorithm>
#include "xstd/Assert.h"
#include "Hapy/Buffer.h"
#include "Hapy/Rules.h"
#include "Hapy/Parser.h"

using namespace Hapy;


Hapy::Parser::Parser() {
}

void Hapy::Parser::grammar(const Rule &aStart) {
	theStartRule = aStart;
	//theSkipRule = aSkipper;
}

const Result &Hapy::Parser::result() const {
	return theResult;
}

bool Hapy::Parser::parse(const string &content) {
	if (begin() && step(content, true))
		end();
#if CERR_DEBUG
cerr << here << "final sc: " << theResult.statusCode.sc() << " == " << Result::scMatch << endl;
cerr << here << "final imageSize: " << theResult.pree.rawImageSize() << " == " << content.size() << endl;
#endif
	return theResult.statusCode == Result::scMatch &&
		theResult.pree.rawImageSize() == content.size();
}

bool Hapy::Parser::begin() {
	// we do not support multiple calls to begin() because we do not reset()
	Should(!theResult.known());

	if (theSkipRule.hasAlg())
		theBuffer.skipper(theSkipRule);

	Should(theBuffer.empty());
	theResult.statusCode = theStartRule.firstMatch(theBuffer, theResult.pree);
#if CERR_DEBUG
cerr << here << "begin() fm(): " << theResult.statusCode.sc() << endl;
#endif
	// the first application is likey to return scMore; can also succeed or err
	if (theResult.statusCode == Result::scMore)
		return true;

	return last();
}

bool Hapy::Parser::step(const string &content, bool lastStep) {
	if (!Should(theResult.statusCode != Result::scError)) {
		// user ignored an earlier error
		return false;
	}

	if (!Should(theStartRule.hasAlg())) {
		// missing grammar
		theResult.statusCode = Result::scError;
		return last();
	}

	if (!Should(theResult.statusCode == Result::scMore)) {
		// unexpected or unknown state, steps() should be called iff more()
		return last();
	}

	theBuffer.append(content);
	theBuffer.atEnd(lastStep);
#if CERR_DEBUG
cerr << here << "buffer:" << theBuffer.content() << endl;
#endif

	theResult.statusCode = theStartRule.resume(theBuffer, theResult.pree);

	if (theResult.statusCode == Result::scMore)
		return true;

	return last();
}

bool Hapy::Parser::more() const {
	return theResult.statusCode == Result::scMore;
}

bool Hapy::Parser::end() {
	if (theResult.statusCode == Result::scMore)
		step(string(), true);
	return last();
}

bool Hapy::Parser::last() {
	switch (theResult.statusCode.sc()) {
		case Result::scError:
		case Result::scMatch:
		case Result::scMiss:
		case Result::scMore:
			break;
		case Result::scNone:
			Should(false); // begin() was not called
			theResult.statusCode = Result::scError;
			break;
		default:
			Should(false); // unknown state
			theResult.statusCode = Result::scError;
	}
	return false;
}

