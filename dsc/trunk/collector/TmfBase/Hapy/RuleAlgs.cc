 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include <functional>
#include <algorithm>
#include "xstd/Assert.h"
#include "xstd/ForeignMemFun.h"
#include "xstd/Bind2Ref.h"

#include "Hapy/Buffer.h"
#include "Hapy/Parser.h"
#include "Hapy/RuleAlgs.h"

using namespace Hapy;

static RuleAlg::StatusCode ApplyRule(const Rule &r, Buffer &buf, PreeNode &pree, const char *defName = "");
static void PrintSubRule(ostream &os, const Rule &r);


// r = empty sequence
RuleAlg::StatusCode Hapy::EmptyRule::firstMatch(Buffer &, PreeNode &) const {
	return Result::scMatch;
}
	
RuleAlg::StatusCode Hapy::EmptyRule::nextMatch(Buffer &, PreeNode &) const {
	return Result::scMiss;
}

RuleAlg::StatusCode Hapy::EmptyRule::resume(Buffer &, PreeNode &) const {
	Should(false);
	return Result::scError;
}

bool Hapy::EmptyRule::terminal() const {
	return true;
}

bool Hapy::EmptyRule::compile() {
	return true;
}

void Hapy::EmptyRule::spreadTrim(const Rule &) {
}

ostream &Hapy::EmptyRule::print(ostream &os) const {
	return os << "empty";
}


// r = a >> b
Hapy::SeqRule::SeqRule() {
}

void Hapy::SeqRule::add(const Rule &rule) {
	theRules.push_back(rule);
}

void Hapy::SeqRule::addMany(const SeqRule &rule) {
	theRules.insert(theRules.end(), rule.theRules.begin(), rule.theRules.end());
}

RuleAlg::StatusCode Hapy::SeqRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 0);
	return advance(buf, pree);
}

// test remaining subrules
RuleAlg::StatusCode Hapy::SeqRule::advance(Buffer &buf, PreeNode &pree) const {
	while (pree.rawCount() < theRules.size()) {
		const Store::size_type idx = pree.rawCount();
		switch (ApplyRule(theRules[idx], buf, pree.newChild(), ">").sc()) {
			case Result::scMatch:
				break; // switch, not loop
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
	return Result::scMatch;
}

RuleAlg::StatusCode Hapy::SeqRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == theRules.size());
	return nextMatchTail(buf, pree);
}

// get other matches starting with the current subrule
RuleAlg::StatusCode Hapy::SeqRule::nextMatchTail(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() <= theRules.size());
	if (pree.rawCount() <= 0)
		return Result::scMiss;
	const int idx = pree.rawCount() - 1;
	switch (theRules[idx].nextMatch(buf, pree.backChild()).sc()) {
		case Result::scMatch:
			return advance(buf, pree);
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

// current subrule failed: kill its result, search for other earlier matches
RuleAlg::StatusCode Hapy::SeqRule::backtrack(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() <= theRules.size());
	Assert(pree.rawCount() > 0);
	pree.popChild();
	if (pree.rawCount() <= 0)
		return Result::scMiss;
	return nextMatchTail(buf, pree);
}

RuleAlg::StatusCode Hapy::SeqRule::resume(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() <= theRules.size());
	Assert(pree.rawCount() > 0);
	const int idx = pree.rawCount() - 1;
	switch (theRules[idx].resume(buf, pree.backChild()).sc()) {
		case Result::scMatch:
			return advance(buf, pree);
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

bool Hapy::SeqRule::isA(const string &s) const {
	return s == "SeqRule";
}
		
bool Hapy::SeqRule::terminal() const {
	return theRules.size() == 0;
}

bool Hapy::SeqRule::compile() {
	for (Store::iterator i = theRules.begin(); i != theRules.end(); ++i) {
		if (!i->compile())
			return false;
	}
	return true;
}

void Hapy::SeqRule::spreadTrim(const Rule &itrimmer) {
	bool odd;
	for (Store::iterator i = theRules.begin(); i != theRules.end(); ++i) {
		if (odd)
			i->implicitTrim(itrimmer);
		else
			i->implicitTrim(itrimmer); // XXX: optimize with internalImplTrim
		odd = !odd;
	}
}

ostream &Hapy::SeqRule::print(ostream &os) const {
	for (Store::const_iterator i = theRules.begin(); i != theRules.end(); ++i) {
		if (i != theRules.begin())
			os << " >> ";
		PrintSubRule(os, *i);
	}
	return os;
}


// r = a | b
void Hapy::OrRule::add(const Rule &rule) {
	theRules.push_back(rule);
}

void Hapy::OrRule::addMany(const OrRule &rule) {
	theRules.insert(theRules.end(), rule.theRules.begin(), rule.theRules.end());
}

RuleAlg::StatusCode Hapy::OrRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 0);
	pree.idata = 0;
	return advance(buf, pree);
}

// find next matching subrule
RuleAlg::StatusCode Hapy::OrRule::advance(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 0);
	Assert(0 <= pree.idata && pree.idata <= theRules.size());
	if (pree.idata < theRules.size()) {
		switch (ApplyRule(theRules[pree.idata], buf, pree.newChild(), "|").sc()) {
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

RuleAlg::StatusCode Hapy::OrRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	return nextMatchTail(buf, pree);
}

// look for other matches starting with the current subrule
RuleAlg::StatusCode Hapy::OrRule::nextMatchTail(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 1);
	Assert(0 <= pree.idata && pree.idata <= theRules.size());
	if (pree.idata < theRules.size()) {
		switch (theRules[pree.idata].nextMatch(buf, pree.backChild()).sc()) {
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
RuleAlg::StatusCode Hapy::OrRule::backtrack(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 1);
	pree.popChild();
	++pree.idata;
	return advance(buf, pree);
}

RuleAlg::StatusCode Hapy::OrRule::resume(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 1);
	Assert(0 <= pree.idata && pree.idata < theRules.size());
	switch (theRules[pree.idata].resume(buf, pree.backChild()).sc()) {
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

bool Hapy::OrRule::isA(const string &s) const {
	return s == "OrRule";
}
		
bool Hapy::OrRule::terminal() const {
	return theRules.size() == 0;
}

bool Hapy::OrRule::compile() {
	for (Store::iterator i = theRules.begin(); i != theRules.end(); ++i) {
		if (!i->compile())
			return false;
	}
	return true;
}

void Hapy::OrRule::spreadTrim(const Rule &itrimmer) {
	for (Store::iterator i = theRules.begin(); i != theRules.end(); ++i)
		i->implicitTrim(itrimmer);
}

ostream &Hapy::OrRule::print(ostream &os) const {
	for (Store::const_iterator i = theRules.begin(); i != theRules.end(); ++i) {
		if (i != theRules.begin())
			os << " | ";
		PrintSubRule(os, *i);
	}
	return os;
}

// r = a - b
Hapy::DiffRule::DiffRule(const Rule &aMatch, const Rule &anExcept): theMatch(aMatch),
	theExcept(anExcept) {
}

RuleAlg::StatusCode Hapy::DiffRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	pree.newChild();
	pree.idata = 0;
	return checkAndAdvance(buf, pree,
		theExcept.firstMatch(buf, pree.backChild()));
}

RuleAlg::StatusCode Hapy::DiffRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	// we already know that theExcept did not match
	Should(pree.idata == 1);
	return theMatch.nextMatch(buf, pree.backChild());
}

RuleAlg::StatusCode Hapy::DiffRule::resume(Buffer &buf, PreeNode &pree) const {
	if (pree.idata == 0)
		return checkAndAdvance(buf, pree, theExcept.resume(buf, pree.backChild()));
	else
		return theMatch.resume(buf, pree.backChild());
}

RuleAlg::StatusCode Hapy::DiffRule::checkAndAdvance(Buffer &buf, PreeNode &pree, StatusCode res) const {
	switch (res.sc()) {
		case Result::scMatch:
			theExcept.cancel(buf, pree.backChild());
			return Result::scMiss;
		case Result::scMore:
			return Result::scMore;
		case Result::scMiss:
			pree.idata = 1;
			pree.popChild();
			pree.newChild();
			return theMatch.firstMatch(buf, pree.backChild());
		case Result::scError:
			return Result::scError;
		default:
			Should(false); // unknown code
			return Result::scError;
	}
}

bool Hapy::DiffRule::terminal() const {
	return false;
}

bool Hapy::DiffRule::compile() {
	return theExcept.compile() && theMatch.compile();
}

void Hapy::DiffRule::spreadTrim(const Rule &r) {
	theExcept.implicitTrim(r);
	theMatch.implicitTrim(r);
}

ostream &Hapy::DiffRule::print(ostream &os) const {
	PrintSubRule(os, theMatch);
	os << " - ";
	PrintSubRule(os, theExcept);
	return os;
}


// r = *a
Hapy::StarRule::StarRule(const Rule &aRule): theRule(aRule) {
}

RuleAlg::StatusCode Hapy::StarRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 0);
	// be greedy but remember that an empty sequence always matches
	const RuleAlg::StatusCode res = tryMore(buf, pree);
	return res == Result::scMiss ? Result::scMatch : res;
}
	
RuleAlg::StatusCode Hapy::StarRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	if (pree.rawCount() > 0) // empty sequence did not match yet
		return checkAndTry(buf, pree, theRule.nextMatch(buf, pree.backChild()));
	return Result::scMiss;
}

RuleAlg::StatusCode Hapy::StarRule::backtrack(Buffer &buf, PreeNode &pree) const {
	if (!Should(pree.rawCount() > 0)) // empty sequence have not match yet
		return Result::scError;

	// get rid of the failed node
	pree.popChild();

	// try the same sequence but without the last node
	// this covers the case of an empty sequence as well
	return Result::scMatch;
}

RuleAlg::StatusCode Hapy::StarRule::resume(Buffer &buf, PreeNode &pree) const {
	if (!Should(pree.rawCount() > 0)) // empty sequence did not match yet
		return Result::scError;
	switch (theRule.resume(buf, pree.backChild()).sc()) {
		case Result::scMatch: {
			const RuleAlg::StatusCode moreRes = tryMore(buf, pree);
			return moreRes == Result::scMiss ? Result::scMatch : moreRes;
		}
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

RuleAlg::StatusCode Hapy::StarRule::checkAndTry(Buffer &buf, PreeNode &pree, StatusCode res) const {
	switch (res.sc()) {
		case Result::scMatch: {
			const RuleAlg::StatusCode moreRes = tryMore(buf, pree);
			return moreRes == Result::scMiss ? res : moreRes;
		}
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

RuleAlg::StatusCode Hapy::StarRule::tryMore(Buffer &buf, PreeNode &pree) const {
	int count = 0;
	RuleAlg::StatusCode res;
	while ((res = ApplyRule(theRule, buf, pree.newChild(), "*")) == Result::scMatch)
		count++;

	if (res != Result::scMiss)
		return res;

	// get rid of the last failed node on failure
	pree.popChild();

	return count > 1 ? Result::scMatch : Result::scMiss;
}

bool Hapy::StarRule::terminal() const {
	return false;
}

bool Hapy::StarRule::compile() {
	return theRule.compile();
}

void Hapy::StarRule::spreadTrim(const Rule &r) {
	theRule.implicitTrim(r);
}

ostream &Hapy::StarRule::print(ostream &os) const {
	PrintSubRule(os << "*", theRule);
	return os;
}


// r = a
Hapy::ProxyRule::ProxyRule(const Rule &aRule): theRule(aRule) {
}

RuleAlg::StatusCode Hapy::ProxyRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	Should(pree.rawCount() == 0);
	return check(buf, pree, theRule.firstMatch(buf, pree.newChild()));
}

RuleAlg::StatusCode Hapy::ProxyRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	Should(pree.rawCount() == 1);
	return check(buf, pree, theRule.nextMatch(buf, pree.backChild()));
}

RuleAlg::StatusCode Hapy::ProxyRule::resume(Buffer &buf, PreeNode &pree) const {
	Should(pree.rawCount() == 1);
	return check(buf, pree, theRule.resume(buf, pree.backChild()));
}

RuleAlg::StatusCode Hapy::ProxyRule::backtrack(Buffer &buf, PreeNode &pree) const {
	if (!Should(pree.rawCount() > 0)) // empty sequence have not match yet
		return Result::scError;

	// get rid of the failed node
	pree.popChild();
	return Result::scMiss;
}

RuleAlg::StatusCode Hapy::ProxyRule::check(Buffer &buf, PreeNode &pree, StatusCode res) const {
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

bool Hapy::ProxyRule::terminal() const {
	return false;
}

bool Hapy::ProxyRule::compile() {
	return theRule.compile();
}

void Hapy::ProxyRule::spreadTrim(const Rule &r) {
	theRule.implicitTrim(r);
}

ostream &Hapy::ProxyRule::print(ostream &os) const {
	PrintSubRule(os << "=", theRule);
	return os;
}


// r = "string"
Hapy::StringRule::StringRule(const string &aToken): theToken(aToken) {
}

RuleAlg::StatusCode Hapy::StringRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	return resume(buf, pree);
}

RuleAlg::StatusCode Hapy::StringRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	buf.backtrack(theToken.size());
	pree.image(string());
	return Result::scMiss;
}

RuleAlg::StatusCode Hapy::StringRule::resume(Buffer &buf, PreeNode &pree) const {
	if (buf.contentSize() < theToken.size()) {
		if (buf.atEnd())
			return Result::scMiss;
		return (buf.contentSize() > 0 &&
			buf.content().compare(theToken, 0, buf.contentSize()) != 0) ?
			Result::scMiss : Result::scMore; // need_more_input
	}
	if (buf.content().compare(theToken, 0, theToken.size()) != 0) {
		return Result::scMiss;
	}
	buf.advance(theToken.size());
	pree.image(theToken);
	Should(pree.rawCount() == 0);
	return Result::scMatch;
}

bool Hapy::StringRule::terminal() const {
	return true;
}

bool Hapy::StringRule::compile() {
	return true;
}

void Hapy::StringRule::spreadTrim(const Rule &) {
}

ostream &Hapy::StringRule::print(ostream &os) const {
	os << '"' << theToken << '"';
	return os;
}

// r = single character from a known set
Hapy::CharSetRule::CharSetRule(const string &aSetName): theSetName(aSetName) {
}

RuleAlg::StatusCode Hapy::CharSetRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	return resume(buf, pree);
}

RuleAlg::StatusCode Hapy::CharSetRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	buf.backtrack(1);
	pree.image(string());
	return Result::scMiss;
}

RuleAlg::StatusCode Hapy::CharSetRule::resume(Buffer &buf, PreeNode &pree) const {
	if (buf.contentSize() <= 0) {
		return buf.atEnd() ? Result::scMiss : Result::scMore;
	}

	const char c = buf.peek();
	if (!matchingChar(c)) {
		return Result::scMiss;
	}

	buf.advance(1);
	pree.image(string(1, c)); // optimize with cached strings?
	return Result::scMatch;
}

bool Hapy::CharSetRule::terminal() const {
	return true;
}

bool Hapy::CharSetRule::compile() {
	return true;
}

void Hapy::CharSetRule::spreadTrim(const Rule &) {
}

ostream &Hapy::CharSetRule::print(ostream &os) const {
	return os << theSetName;
}


// r = any char
Hapy::AnyCharRule::AnyCharRule(): CharSetRule("anychar") {
}

bool Hapy::AnyCharRule::matchingChar(char) const {
	return true;
}


// r = alpha
Hapy::AlphaRule::AlphaRule(): CharSetRule("alpha") {
}

bool Hapy::AlphaRule::matchingChar(char c) const {
	return isalpha(c);
}


// r = digit
Hapy::DigitRule::DigitRule(): CharSetRule("digit") {
}

bool Hapy::DigitRule::matchingChar(char c) const {
	return isdigit(c);
}


// r = space
Hapy::SpaceRule::SpaceRule(): CharSetRule("white") {
}

bool Hapy::SpaceRule::matchingChar(char c) const {
	return isspace(c);
}


// r = end of input
RuleAlg::StatusCode Hapy::EndRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	return resume(buf, pree);
}
	
RuleAlg::StatusCode Hapy::EndRule::nextMatch(Buffer &, PreeNode &) const {
	return Result::scMiss;
}

RuleAlg::StatusCode Hapy::EndRule::resume(Buffer &buf, PreeNode &pree) const {
	if (!buf.empty())
		return Result::scMiss;
	if (!buf.atEnd())
		return Result::scMore;
	return Result::scMatch;
}

bool Hapy::EndRule::terminal() const {
	return true;
}

bool Hapy::EndRule::compile() {
	return true;
}

void Hapy::EndRule::spreadTrim(const Rule &) {
}

ostream &Hapy::EndRule::print(ostream &os) const {
	return os << "end";
}

#if 0
Hapy::AggrRule::AggrRule(const Rule &aRule): theRule(aRule) {
}

RuleAlg::StatusCode Hapy::AggrRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 0);
	buf.prep();
	buf.pauseSkip();
	return finish(buf, pree, theRule.apply(buf, pree.newChild()));
}

RuleAlg::StatusCode Hapy::AggrRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	Assert(pree.rawCount() == 1);
	buf.prep();
	buf.pauseSkip();
	return finish(buf, pree, theRule.advance(buf, pree.backChild()));
}

bool Hapy::AggrRule::finish(Buffer &buf, PreeNode &pree, bool res) const {
	Assert(pree.rawCount() == 1);
	buf.resumeSkip();
	pree.image(string());
	if (res)
		pree.backChild().aggrImage(pree.image);
	else
		pree.popChild();
	return res;
}

bool Hapy::AggrRule::finalize(PreeNode &pree) const {
	string i;
	pree.aggrImage(i);
	pree.image(i);
	pree.children.clear();
	return true;
}

ostream &Hapy::AggrRule::print(ostream &os) const {
	PrintSubRule(os, theRule);
	return os;
}


Hapy::CommitRule::CommitRule(const Rule &aRule): theRule(aRule) {
}

RuleAlg::StatusCode Hapy::CommitRule::firstMatch(Buffer &buf, PreeNode &pree) const {
	if (theRule.apply(buf, pree.newChild()))
		return Result::scMatch;
	pree.popChild();
	return false;
}

RuleAlg::StatusCode Hapy::CommitRule::nextMatch(Buffer &buf, PreeNode &pree) const {
	theRule.cancel(buf, pree.backChild());
	pree.popChild();
	return false;
}

inline
void subFinalize(PreeNode &pree, PreeNode::Children::iterator i, const Rule &sub) {
	Assert(false); // if (!sub.finalize(*i))
	pree.children.erase(i);
}

bool Hapy::CommitRule::finalize(PreeNode &pree) const {
	Assert(pree.rawCount() == 1);
	subFinalize(pree, pree.children.begin(), theRule);
	if (Should(pree.rawCount() == 1))
		pree = pree.backChild();
	return true;
}

ostream &Hapy::CommitRule::print(ostream &os) const {
	PrintSubRule(os, theRule);
	return os;
}
#endif

inline
string shortStr(const string &str) {
	if (str.size() > 40)
		return str.substr(0, 40);
	else
		return str;
}

static
RuleAlg::StatusCode ApplyRule(const Rule &r, Buffer &buf, PreeNode &pree, const char *defName) {
#if HAPY_DEBUG
	static int lastId = 0;
	static int level = 0;
	const int id = ++lastId;
	++level;
	const string pfx = string(2*level, ' ');
	const char *name = r.name().size() ? r.name().c_str() : defName;
r.print(cerr << endl << id << '/' << level << '-' << pfx << "try rule: ") << " (" << name << "/" << defName << ")" << endl;
cerr << id << '/' << level << '-' << pfx << "try buffer: " << shortStr(buf.content()) << ". end: " << buf.contentSize() << " ? " << buf.atEnd() << endl;
cerr << id << '/' << level << '-' << pfx << "rid: " << pree.rawRid() << " pree: " << &pree << " children: " << pree.rawCount() << endl;
#endif
	const RuleAlg::StatusCode res = r.firstMatch(buf, pree);
#if HAPY_DEBUG
r.print(cerr << endl << id << '/' << level << '-' << pfx << '#' << res.sc() << " rule: ") << " (" << name << "/" << defName << ")" << endl;
cerr << id << '/' << level << '-' << pfx << '#' << res.sc() << " buffer: " << shortStr(buf.content()) << ". end: " << buf.contentSize() << " ? " << buf.atEnd() << endl;
cerr << id << '/' << level << '-' << pfx << "rid: " << pree.rawRid() << " pree: " << &pree << " children: " << pree.rawCount() << endl;
	--level;
#endif
	return res;
}

static
void PrintSubRule(ostream &os, const Rule &r) {
	if (r.name().size()) {
		os << r.name();
	} else
	if (r.hasAlg()) {
		os << '(';
		r.alg().print(os);
		os << ')';
	}
}
