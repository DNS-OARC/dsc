/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Buffer.h>
#include <Hapy/RuleBase.h>
#include <Hapy/Rule.h>
#include <Hapy/Rules.h>
#include <Hapy/Debugger.h>
#include <Hapy/Parser.h>

#include <functional>
#include <algorithm>


// keep in sync with moveOn()
Hapy::Parser::Parser(): theStartRule(0), isCompiled(false) {
}

void Hapy::Parser::grammar(const Rule &aStart) {
	theStartRule = aStart.base();
}

const Hapy::Result &Hapy::Parser::result() const {
	return theResult;
}

bool Hapy::Parser::parse(const string &content) {
	// allow repeated parse() calls; keep in sync with ctors
	if (theResult.statusCode != Result::scNone) {
		theBuffer.reset();
		theResult = Result();
		// compilation-related members are preserved as is
	}

	pushData(content);
	sawDataEnd(true);

	if (!isCompiled) {
		theCflags.reachEnd = true;
		if (!compile()) {
			theResult.statusCode = Result::scError;
			return last();
		}
	}

	begin();
	end();
	return theResult.statusCode == Result::scMatch;
}

bool Hapy::Parser::nextMatch() {
	Should(theResult.statusCode == Result::scMatch);
	theResult.statusCode = theStartRule->nextMatch(theBuffer, theResult.pree);
	last();
	return theResult.statusCode == Result::scMatch;
}

bool Hapy::Parser::begin() {
	// call moveOn() first if you want to call begin() after end()
	if (!Should(!theResult.known())) {
		theResult.statusCode = Result::scError;
		return last();
	}

	if (!isCompiled && !compile()) {
		theResult.statusCode = Result::scError;
		return last();
	}

	theResult.statusCode = theStartRule->firstMatch(theBuffer, theResult.pree);
	return theResult.statusCode == Result::scMore;
}

bool Hapy::Parser::step() {
	if (!Should(theResult.statusCode != Result::scError)) {
		// user ignored an earlier error
		return false;
	}

	if (!Should(theResult.statusCode == Result::scMore)) {
		// unexpected or unknown state, steps() should be called iff scMore
		return last();
	}

	theResult.statusCode = theStartRule->resume(theBuffer, theResult.pree);

	if (theResult.statusCode == Result::scMore)
		return true;

	return last();
}

bool Hapy::Parser::end() {
	if (theResult.statusCode == Result::scMore) {
		sawDataEnd(true);
		step();
	}
	return last();
}

void Hapy::Parser::pushData(const string &newData) {
	theBuffer.append(newData);
}

bool Hapy::Parser::hasData() const {
	return !theBuffer.empty();
}

bool Hapy::Parser::sawDataEnd() const {
	return theBuffer.sawEnd();
}

void Hapy::Parser::sawDataEnd(bool did) {
	theBuffer.sawEnd(did);
}

// keep in sync with ctor
void Hapy::Parser::moveOn() {
	theBuffer.moveOn();
	theResult = Result();
	// compilation-related members (e.g., theCflags, theStartRule, isCompiled)
	// are preserved as is
}

bool Hapy::Parser::last() {
	theResult.maxPos = theBuffer.maxProgress().size();
	theResult.input = theBuffer.allContent();
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

bool Hapy::Parser::compile() {
	Debugger::Configure();

	if (Should(theStartRule) &&
		Should(theStartRule->build(theCflags))) {
		isCompiled = true;
		return true;
	}

	return false;
}
