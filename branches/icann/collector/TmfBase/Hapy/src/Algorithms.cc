/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Buffer.h>
#include <Hapy/Parser.h>
#include <Hapy/Algorithms.h>
#include <Hapy/RuleBase.h>
#include <Hapy/IoStream.h>

#include <functional>
#include <algorithm>

namespace Hapy {
static
void PrintSubRule(ostream &os, const RulePtr &r) {
	if (r->id().known())
		os << r->id();
	else
		os << "(...)";
}
}

// r = empty sequence
Hapy::Algorithm::StatusCode Hapy::EmptyAlg::firstMatch(Buffer &, Pree &) const {
	return Result::scMatch;
}
	
Hapy::Algorithm::StatusCode Hapy::EmptyAlg::nextMatch(Buffer &, Pree &) const {
	return Result::scMiss;
}

Hapy::Algorithm::StatusCode Hapy::EmptyAlg::resume(Buffer &, Pree &) const {
	Should(false);
	return Result::scError;
}

bool Hapy::EmptyAlg::terminal(string *name) const {
	static const string n = "empty";
	if (name)
		*name = n;
	return true;
}

bool Hapy::EmptyAlg::compile(const RulePtr &r) {
	return true;
}

Hapy::ostream &Hapy::EmptyAlg::print(ostream &os) const {
	return os << "empty";
}


// r = a >> b
Hapy::SeqAlg::SeqAlg() {
}

void Hapy::SeqAlg::add(const RulePtr &rule) {
	theAlgs.push_back(rule);
}

void Hapy::SeqAlg::addMany(const SeqAlg &rule) {
	theAlgs.insert(theAlgs.end(), rule.theAlgs.begin(), rule.theAlgs.end());
}

Hapy::Algorithm::StatusCode Hapy::SeqAlg::firstMatch(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 0);
	return advance(buf, pree);
}

Hapy::Algorithm::StatusCode Hapy::SeqAlg::nextMatch(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == theAlgs.size());
	const StatusCode res = backtrack(buf, pree);
	if (res != Result::scMatch)
		return res;
	return advance(buf, pree);
}

Hapy::Algorithm::StatusCode Hapy::SeqAlg::resume(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() <= theAlgs.size());
	Assert(pree.rawCount() > 0);
	const int idx = pree.rawCount() - 1;
	switch (theAlgs[idx]->resume(buf, pree.backChild()).sc()) {
		case Result::scMatch:
			return advance(buf, pree);
		case Result::scMore:
			return Result::scMore;
		case Result::scMiss: {
			killCurrent(buf, pree);
			const StatusCode res = backtrack(buf, pree);
			if (res != Result::scMatch)
				return res;
			return advance(buf, pree);
		}
		case Result::scError:
			return Result::scError;
		default:
			Should(false); // unknown code
			return Result::scError;
	}
}

// move right: find remaining matches
Hapy::Algorithm::StatusCode Hapy::SeqAlg::advance(Buffer &buf, Pree &pree) const {
	while (pree.rawCount() < theAlgs.size()) {
		const Store::size_type idx = pree.rawCount();
		switch (theAlgs[idx]->firstMatch(buf, pree.newChild()).sc()) {
			case Result::scMatch:
				break; // switch, not loop
			case Result::scMiss: {
				killCurrent(buf, pree);
				const StatusCode res = backtrack(buf, pree);
				if (res != Result::scMatch)
					return res;
				break; // switch, not loop
			}
			case Result::scMore:
				return Result::scMore;
			case Result::scError:
				return Result::scError;
			default:
				Should(false); // unknown code
				return Result::scError;
		}
	}
	return Result::scMatch;
}

// move left: find another match among already matched subrules
Hapy::Algorithm::StatusCode Hapy::SeqAlg::backtrack(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() <= theAlgs.size());
	while (pree.rawCount() > 0) {
		const int idx = pree.rawCount() - 1;
		const StatusCode res =
			theAlgs[idx]->nextMatch(buf, pree.backChild()).sc();
		if (res == Result::scMiss)
			killCurrent(buf, pree);
		else
			return res;
	}
	return Result::scMiss;
}

// current subrule failed: kill its result
void Hapy::SeqAlg::killCurrent(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() <= theAlgs.size());
	Assert(pree.rawCount() > 0);
	pree.popChild();
}

bool Hapy::SeqAlg::isA(const string &s) const {
	return s == "Seq";
}
		
bool Hapy::SeqAlg::terminal(string *) const {
	return theAlgs.size() == 0;
}

bool Hapy::SeqAlg::compile(const RulePtr &r) {
	for (Store::iterator i = theAlgs.begin(); i != theAlgs.end(); ++i) {
		if (!compileSubRule(*i, r))
			return false;
	}
	return true;
}

Hapy::ostream &Hapy::SeqAlg::print(ostream &os) const {
	for (Store::const_iterator i = theAlgs.begin(); i != theAlgs.end(); ++i) {
		if (i != theAlgs.begin())
			os << " >> ";
		PrintSubRule(os, *i);
	}
	return os;
}


// r = a | b
void Hapy::OrAlg::add(const RulePtr &rule) {
	theAlgs.push_back(rule);
}

void Hapy::OrAlg::addMany(const OrAlg &rule) {
	theAlgs.insert(theAlgs.end(), rule.theAlgs.begin(), rule.theAlgs.end());
}

Hapy::Algorithm::StatusCode Hapy::OrAlg::firstMatch(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 0);
	pree.idata = 0;
	return advance(buf, pree);
}

// find next matching subrule
Hapy::Algorithm::StatusCode Hapy::OrAlg::advance(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 0);
	Assert(0 <= pree.idata && pree.idata <= theAlgs.size());

	// find untried alternative to avoid left recursion
	while (pree.idata < theAlgs.size() && pree.leftRecursion())
		++pree.idata;

	if (pree.idata < theAlgs.size()) {
		switch (theAlgs[pree.idata]->firstMatch(buf, pree.newChild()).sc()) {
			case Result::scMatch:
				return Result::scMatch;
			case Result::scMore:
				return Result::scMore;
			case Result::scMiss:
				return backtrack(buf, pree); // will call us back
			case Result::scError:
				return Result::scError;
			default:
				Should(false); // unknown code
				return Result::scError;
		}
	}

	return Result::scMiss;
}

Hapy::Algorithm::StatusCode Hapy::OrAlg::nextMatch(Buffer &buf, Pree &pree) const {
	return nextMatchTail(buf, pree);
}

// look for other matches starting with the current subrule
Hapy::Algorithm::StatusCode Hapy::OrAlg::nextMatchTail(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 1);
	Assert(0 <= pree.idata && pree.idata <= theAlgs.size());
	if (pree.idata < theAlgs.size()) {
		switch (theAlgs[pree.idata]->nextMatch(buf, pree.backChild()).sc()) {
			case Result::scMatch:
				return Result::scMatch;
			case Result::scMore:
				return Result::scMore;
			case Result::scMiss:
				return backtrack(buf, pree);
			case Result::scError:
				return Result::scError;
			default:
				Should(false); // unknown code
				return Result::scError;
		}
	}
	return Result::scMiss;
}

// current subrule failed: kill its result, search for later matches
Hapy::Algorithm::StatusCode Hapy::OrAlg::backtrack(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 1);
	pree.popChild();
	++pree.idata;
	return advance(buf, pree);
}

Hapy::Algorithm::StatusCode Hapy::OrAlg::resume(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 1);
	Assert(0 <= pree.idata && pree.idata < theAlgs.size());
	switch (theAlgs[pree.idata]->resume(buf, pree.backChild()).sc()) {
		case Result::scMatch:
			return Result::scMatch;
		case Result::scMore:
			return Result::scMore;
		case Result::scMiss:
			return backtrack(buf, pree);
		case Result::scError:
			return Result::scError;
		default:
			Should(false); // unknown code
			return Result::scError;
	}
}

bool Hapy::OrAlg::isA(const string &s) const {
	return s == "Or";
}
		
bool Hapy::OrAlg::terminal(string *) const {
	return theAlgs.size() == 0;
}

bool Hapy::OrAlg::compile(const RulePtr &r) {
	for (Store::iterator i = theAlgs.begin(); i != theAlgs.end(); ++i) {
		if (!compileSubRule(*i, r))
			return false;
	}
	return true;
}

Hapy::ostream &Hapy::OrAlg::print(ostream &os) const {
	for (Store::const_iterator i = theAlgs.begin(); i != theAlgs.end(); ++i) {
		if (i != theAlgs.begin())
			os << " | ";
		PrintSubRule(os, *i);
	}
	return os;
}

// r = a - b
Hapy::DiffAlg::DiffAlg(const RulePtr &aMatch, const RulePtr &anExcept): theMatch(aMatch),
	theExcept(anExcept) {
}

Hapy::Algorithm::StatusCode Hapy::DiffAlg::firstMatch(Buffer &buf, Pree &pree) const {
	pree.newChild();
	pree.idata = 0;
	return checkAndAdvance(buf, pree,
		theExcept->firstMatch(buf, pree.backChild()));
}

Hapy::Algorithm::StatusCode Hapy::DiffAlg::nextMatch(Buffer &buf, Pree &pree) const {
	// we already know that theExcept did not match
	Should(pree.idata == 1);
	return theMatch->nextMatch(buf, pree.backChild());
}

Hapy::Algorithm::StatusCode Hapy::DiffAlg::resume(Buffer &buf, Pree &pree) const {
	if (pree.idata == 0)
		return checkAndAdvance(buf, pree, theExcept->resume(buf, pree.backChild()));
	else
		return theMatch->resume(buf, pree.backChild());
}

Hapy::Algorithm::StatusCode Hapy::DiffAlg::checkAndAdvance(Buffer &buf, Pree &pree, StatusCode res) const {
	switch (res.sc()) {
		case Result::scMatch:
			theExcept->cancel(buf, pree.backChild());
			return Result::scMiss;
		case Result::scMore:
			return Result::scMore;
		case Result::scMiss:
			pree.idata = 1;
			pree.popChild();
			pree.newChild();
			return theMatch->firstMatch(buf, pree.backChild());
		case Result::scError:
			return Result::scError;
		default:
			Should(false); // unknown code
			return Result::scError;
	}
}

bool Hapy::DiffAlg::terminal(string *) const {
	return false;
}

bool Hapy::DiffAlg::compile(const RulePtr &r) {
	return compileSubRule(theExcept, r) && compileSubRule(theMatch, r);
}

Hapy::ostream &Hapy::DiffAlg::print(ostream &os) const {
	PrintSubRule(os, theMatch);
	os << " - ";
	PrintSubRule(os, theExcept);
	return os;
}


// r = {min,max}a
Hapy::ReptionAlg::ReptionAlg(const RulePtr &aAlg, size_type aMin, size_type aMax):
	theAlg(aAlg), theMin(aMin), theMax(aMax) {
	Should(theMin <= theMax);
}

Hapy::Algorithm::StatusCode Hapy::ReptionAlg::firstMatch(Buffer &buf, Pree &pree) const {
	Assert(pree.rawCount() == 0);
	return tryMore(buf, pree);
}
	
Hapy::Algorithm::StatusCode Hapy::ReptionAlg::nextMatch(Buffer &buf, Pree &pree) const {
	return pree.rawCount() == 0 ? // has empty sequence matched?
		Result::scMiss :
		checkAndTry(buf, pree, theAlg->nextMatch(buf, pree.backChild()));
}

Hapy::Algorithm::StatusCode Hapy::ReptionAlg::backtrack(Buffer &buf, Pree &pree) const {
	if (pree.rawCount() == 0) // empty sequence has already match
		return Result::scMiss;

	// get rid of the failed node
	pree.popChild();

	// change the tail and try to grow to reach theMin goal if needed
	// otherwise, try the same sequence but now without the last node
	// the latter covers the case of an empty sequence as well
	return pree.rawCount() < theMin ? nextMatch(buf, pree) : Result::scMatch;
}

Hapy::Algorithm::StatusCode Hapy::ReptionAlg::resume(Buffer &buf, Pree &pree) const {
	if (!Should(pree.rawCount() > 0)) // empty sequence has not match yet
		return Result::scError;
	return checkAndTry(buf, pree, theAlg->resume(buf, pree.backChild()));
}

Hapy::Algorithm::StatusCode Hapy::ReptionAlg::checkAndTry(Buffer &buf, Pree &pree, StatusCode res) const {
	switch (res.sc()) {
		case Result::scMatch:
			return tryMore(buf, pree);
		case Result::scMore:
			return Result::scMore;
		case Result::scMiss:
			return backtrack(buf, pree);
		case Result::scError:
			return Result::scError;
		default:
			Should(false); // unknown code
			return Result::scError;
	}
}

Hapy::Algorithm::StatusCode Hapy::ReptionAlg::tryMore(Buffer &buf, Pree &pree) const {
	Algorithm::StatusCode res = Result::scMatch;
	while (pree.rawCount() < theMax && res == Result::scMatch) {
		res = theAlg->firstMatch(buf, pree.newChild());

		// avoid endless repetition when an empty sequence matches input
		if (res == Result::scMatch && theMax == INT_MAX &&
			pree.rawCount() > theMin && pree.emptyLoop()) {
			theAlg->cancel(buf, pree.backChild());
			res = Result::scMiss;
		}
	}

	if (!Should(pree.rawCount() <= theMax))
		return Result::scError; // too many already

	// res == match implies that count == max, implies that min <= count
	return res == Result::scMiss ? backtrack(buf, pree) : res;
}

bool Hapy::ReptionAlg::terminal(string *) const {
	return theMax == 0;
}

bool Hapy::ReptionAlg::compile(const RulePtr &r) {
	return compileSubRule(theAlg, r);
}

Hapy::ostream &Hapy::ReptionAlg::print(ostream &os) const {
	if (theMax == INT_MAX) {
		if (theMin == 0)
			os << "*";
		else
		if (theMin == 1)
			os << "+";
		else
			os << "{" << theMin << ",}";
	} else {
		os << "{" << theMin << "," << theMax << "}";
	}
	PrintSubRule(os, theAlg);
	return os;
}


// r = a
Hapy::ProxyAlg::ProxyAlg(const RulePtr &aAlg): theAlg(aAlg) {
}

Hapy::Algorithm::StatusCode Hapy::ProxyAlg::firstMatch(Buffer &buf, Pree &pree) const {
	Should(pree.rawCount() == 0);
	return check(buf, pree, theAlg->firstMatch(buf, pree.newChild()));
}

Hapy::Algorithm::StatusCode Hapy::ProxyAlg::nextMatch(Buffer &buf, Pree &pree) const {
	Should(pree.rawCount() == 1);
	return check(buf, pree, theAlg->nextMatch(buf, pree.backChild()));
}

Hapy::Algorithm::StatusCode Hapy::ProxyAlg::resume(Buffer &buf, Pree &pree) const {
	Should(pree.rawCount() == 1);
	return check(buf, pree, theAlg->resume(buf, pree.backChild()));
}

Hapy::Algorithm::StatusCode Hapy::ProxyAlg::backtrack(Buffer &buf, Pree &pree) const {
	if (!Should(pree.rawCount() > 0)) // empty sequence have not match yet
		return Result::scError;

	// get rid of the failed node
	pree.popChild();
	return Result::scMiss;
}

Hapy::Algorithm::StatusCode Hapy::ProxyAlg::check(Buffer &buf, Pree &pree, StatusCode res) const {
	switch (res.sc()) {
		case Result::scMore:
		case Result::scMatch:
			return res;
		case Result::scMiss:
			return backtrack(buf, pree);
		case Result::scError:
			return Result::scError;
		default:
			Should(false); // unknown code
			return Result::scError;
	}
}

bool Hapy::ProxyAlg::terminal(string *) const {
	return false;
}

bool Hapy::ProxyAlg::compile(const RulePtr &r) {
	return compileSubRule(theAlg, r);
}

Hapy::ostream &Hapy::ProxyAlg::print(ostream &os) const {
	PrintSubRule(os << "=", theAlg);
	return os;
}


// r = "string"
Hapy::StringAlg::StringAlg(const string &aToken): theToken(aToken) {
}

Hapy::Algorithm::StatusCode Hapy::StringAlg::firstMatch(Buffer &buf, Pree &pree) const {
	return resume(buf, pree);
}

Hapy::Algorithm::StatusCode Hapy::StringAlg::nextMatch(Buffer &buf, Pree &pree) const {
	buf.backtrack(theToken.size());
	return Result::scMiss;
}

Hapy::Algorithm::StatusCode Hapy::StringAlg::resume(Buffer &buf, Pree &pree) const {
	const string::size_type contentSize = buf.contentSize();
	if (contentSize < theToken.size()) {
		if (buf.sawEnd())
			return Result::scMiss;
		return (contentSize > 0 && !buf.startsWith(theToken)) ?
			Result::scMiss : Result::scMore; // need_more_input
	}
	if (!buf.startsWith(theToken)) {
		return Result::scMiss;
	}
	buf.advance(theToken.size());
	Should(pree.rawCount() == 0);
	return Result::scMatch;
}

bool Hapy::StringAlg::terminal(string *name) const {
	if (name)
		*name = theToken;
	return true;
}

bool Hapy::StringAlg::compile(const RulePtr &r) {
	return true;
}

Hapy::ostream &Hapy::StringAlg::print(ostream &os) const {
	os << '"' << theToken << '"';
	return os;
}

// r = single character from a known set
Hapy::CharSetAlg::CharSetAlg(const string &aSetName): theSetName(aSetName) {
}

Hapy::Algorithm::StatusCode Hapy::CharSetAlg::firstMatch(Buffer &buf, Pree &pree) const {
	return resume(buf, pree);
}

Hapy::Algorithm::StatusCode Hapy::CharSetAlg::nextMatch(Buffer &buf, Pree &pree) const {
	buf.backtrack(1);
	return Result::scMiss;
}

Hapy::Algorithm::StatusCode Hapy::CharSetAlg::resume(Buffer &buf, Pree &pree) const {
	if (buf.contentSize() <= 0) {
		return buf.sawEnd() ? Result::scMiss : Result::scMore;
	}

	const char c = buf.peek();
	if (!matchingChar(c)) {
		return Result::scMiss;
	}

	buf.advance(1);
	return Result::scMatch;
}

bool Hapy::CharSetAlg::terminal(string *name) const {
	if (name)
		*name = theSetName;
	return true;
}

bool Hapy::CharSetAlg::compile(const RulePtr &r) {
	return true;
}

Hapy::ostream &Hapy::CharSetAlg::print(ostream &os) const {
	return os << theSetName;
}


// r = any char
Hapy::AnyCharAlg::AnyCharAlg(): CharSetAlg("anychar") {
}

bool Hapy::AnyCharAlg::matchingChar(char) const {
	return true;
}


// r = user-specified char set
Hapy::SomeCharAlg::SomeCharAlg(const string &set): CharSetAlg("charset"),
	theSet(set.begin(), set.end()) {
}

Hapy::SomeCharAlg::SomeCharAlg(const Set &aSet): CharSetAlg("charset"),
	theSet(aSet) {
}

bool Hapy::SomeCharAlg::matchingChar(char c) const {
	return theSet.find(c) != theSet.end();
}


// r = first <= char <= last
Hapy::CharRangeAlg::CharRangeAlg(char aFirst, char aLast):
	CharSetAlg("char_range"),
	theFirst(aFirst), theLast(aLast) {
	Should(theFirst <= theLast);
}

bool Hapy::CharRangeAlg::matchingChar(char c) const {
	return theFirst <= c && c <= theLast;
}


// r = alpha
Hapy::AlphaAlg::AlphaAlg(): CharSetAlg("alpha") {
}

bool Hapy::AlphaAlg::matchingChar(char c) const {
	return isalpha(c) != 0;
}


// r = digit
Hapy::DigitAlg::DigitAlg(): CharSetAlg("digit") {
}

bool Hapy::DigitAlg::matchingChar(char c) const {
	return isdigit(c) != 0;
}


// r = space
Hapy::SpaceAlg::SpaceAlg(): CharSetAlg("white") {
}

bool Hapy::SpaceAlg::matchingChar(char c) const {
	return isspace(c) != 0;
}


// r = end of input
Hapy::Algorithm::StatusCode Hapy::EndAlg::firstMatch(Buffer &buf, Pree &pree) const {
	return resume(buf, pree);
}
	
Hapy::Algorithm::StatusCode Hapy::EndAlg::nextMatch(Buffer &, Pree &) const {
	return Result::scMiss;
}

Hapy::Algorithm::StatusCode Hapy::EndAlg::resume(Buffer &buf, Pree &pree) const {
	if (!buf.empty())
		return Result::scMiss;
	if (!buf.sawEnd())
		return Result::scMore;
	return Result::scMatch;
}

bool Hapy::EndAlg::terminal(string *name) const {
	static const string n = "end";
	if (name)
		*name = n;
	return true;
}

bool Hapy::EndAlg::compile(const RulePtr &r) {
	return true;
}

Hapy::ostream &Hapy::EndAlg::print(ostream &os) const {
	return os << "end";
}
