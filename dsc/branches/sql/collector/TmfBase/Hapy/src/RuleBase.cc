/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Buffer.h>
#include <Hapy/Parser.h>
#include <Hapy/Algorithms.h>
#include <Hapy/Action.h>
#include <Hapy/RuleBase.h>
#include <Hapy/IoStream.h>
#include <iomanip>

// these static members are used for debugging only
Hapy::RuleBase::DebugDetail Hapy::RuleBase::TheDebugDetail =
	Hapy::RuleBase::dbdNone;
int Hapy::RuleBase::TheLastCallId = 0;
int Hapy::RuleBase::TheCallLevel = 0;


bool Hapy::RuleBase::Debug(DebugDetail codeDetail) {
	return TheDebugDetail >= codeDetail;
}

#ifndef HapyAlgCall
#define HapyAlgCall(alg, buf, pree) \
	call((buf), (pree), &Algorithm::alg, #alg);
#endif



Hapy::RuleBase::RuleBase(): theAlg(0), theAction(0), theTrimmer(0),
	theCommitMode(cmDefault), theTrimMode(tmDefault), isImplicit(false),
	isLeaf(false), mustReachEnd(false), isCompiling(false) {
}

void Hapy::RuleBase::id(const RuleId &anId) {
	theId = anId;
}

void Hapy::RuleBase::committed(bool be) {
	theCommitMode = be ? RuleBase::cmCommit : RuleBase::cmDont;
}

bool Hapy::RuleBase::terminal() const {
	return isLeaf ||
		(Should(isCompiling) && hasAlg() && alg().terminal());
}

// remember explicit trimming request
void Hapy::RuleBase::trim(const RulePtr &trimmer) {
	Should(!theTrimmer);
	theTrimmer = trimmer;
	theTrimMode = RuleBase::tmExplicit;
}

// remember explicit no-internal-trimming request
void Hapy::RuleBase::verbatim(bool be) {
	theTrimMode = RuleBase::tmVerbatim;
}

// remember leaf setting
void Hapy::RuleBase::leaf(bool be) {
	isLeaf = be;
}

void Hapy::RuleBase::reachEnd(bool doIt) {
	mustReachEnd = doIt;
}

// set trimming if no trimming was configured
void Hapy::RuleBase::implicitTrim(const RulePtr &itrimmer) {
	Assert(itrimmer);

	if (theTrimMode == RuleBase::tmDefault) {
		Should(theTrimmer == 0);
		theTrimMode = RuleBase::tmImplicit;
	} else
	if (theTrimMode == RuleBase::tmVerbatim) {
		if (theTrimmer)
			return; // XXX: assume this is the same trimming rule
	} else {
		// honor explicit settings
		return;
	}

	theTrimmer = itrimmer;
}

bool Hapy::RuleBase::compileTrim() {
	if (!Should(theTrimmer))
		return false;

	RulePtr trimmer = theTrimmer;
	theTrimmer = 0;
	if (trimmer->theCommitMode == RuleBase::cmDefault)
		trimmer->committed(true);
	trimmer->leaf(true);
	trimmer->isImplicit = true;

	isCompiling = false; // so that core is not marked as compiled
	RulePtr core(new RuleBase(*this));

	theId = RuleId::Next();
	theId.name(core->id().name() + "_trimmer");
	isImplicit = true;
	isCompiling = true;
	leaf(false);      // but core rule will be if we were
	committed(false); // but core rule will be committed if we were
	theAction = 0;    // but core rule will have it if we had
	theAlg = 0;

	SeqAlg *s = new SeqAlg;
	s->add(trimmer);
	s->add(core);
	s->add(trimmer);
	alg(s);

	return true;
}

bool Hapy::RuleBase::compile(RulePtr itrimmer) {
	// prevent infinite recursion
	if (isCompiling) 
		return true;
	isCompiling = true;

	if (Debug(dbdAll))
		print(clog << this << " pre rule: " << ' ') << endl;

	if (!Should(theAlg))
		return false;

	if (itrimmer)
		implicitTrim(itrimmer);

	if (Debug(dbdAll))
		clog << this << " will " << (shouldTrim() ? "" : "not") << "compile trim" << endl;

	if (shouldTrim() && !compileTrim())
		return false;

	itrimmer = theTrimmer;
	if (theTrimMode == RuleBase::tmVerbatim)
		itrimmer = 0;

	if (Debug(dbdAll))
		clog << this << " will " << (itrimmer ? "" : "not ") << "spread trim" << endl;

	if (!theAlg->compile(itrimmer))
		return false;

	if (Debug(dbdUser)) {
		if (Debug(dbdAll))
			clog << this << ' ';
		print(clog << "compiled rule: " << ' ') << endl;
	}

	return true;
}

bool Hapy::RuleBase::shouldTrim() const {
	if (!theTrimmer)
		return false;

	// trim terminals
	if (terminal())
		return true;

	// trim verbatim nodes because we will not see a terminal behind them
	return theTrimMode == RuleBase::tmVerbatim;
}

bool Hapy::RuleBase::temporary() const {
	return id().temporary();
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::firstMatch(Buffer &buf, Pree &pree) const {
	pree.rawRid(id());
	pree.match.start(buf.parsedSize());
	pree.implicit = isImplicit;
	pree.leaf = isLeaf;
	return HapyAlgCall(firstMatch, buf, pree);
}

// note: HapyAlgCall() may bypass this interface if mustReachEnd is in effect
Hapy::RuleBase::StatusCode Hapy::RuleBase::nextMatch(Buffer &buf, Pree &pree) const {
	Should(pree.rawRid() == id());
	if (theCommitMode == RuleBase::cmCommit) {
		cancel(buf, pree);
		return Result::scMiss;
	}
	return HapyAlgCall(nextMatch, buf, pree);
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::resume(Buffer &buf, Pree &pree) const {
	Should(pree.rawRid() == id());
	return HapyAlgCall(resume, buf, pree);
}

void Hapy::RuleBase::cancel(Buffer &buf, Pree &pree) const {
	Should(pree.rawRid() == id());
	//debug("cancel", buf, pree);
	if (Should(buf.parsedSize() >= pree.match.start()))
		buf.backtrack(buf.parsedSize() - pree.match.start());
}

const Hapy::Algorithm &Hapy::RuleBase::alg() const {
	Assert(theAlg);
	return *theAlg;
}

void Hapy::RuleBase::alg(Algorithm *anAlg) {
	Assert(anAlg);
	Assert(!theAlg); // assume rule redefinitions are bugs
	theAlg = anAlg;
}

void Hapy::RuleBase::action(const Action *anAction) {
	theAction = anAction; // OK to overwrite
}

void Hapy::RuleBase::updateAlg(const RuleBase &src) {
	if (Should(src.hasAlg()))
		theAlg = src.theAlg;
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::applyAction(Buffer &buf, Pree &pree) const {
	StatusCode res = Result::scMatch;
	do {
		Action::Params params(buf, pree, res);
		theAction->act(params);
		switch (res.sc()) {
			case Result::scMatch:
			case Result::scError:
				return res; // we are done
			case Result::scMiss:
				res = HapyAlgCall(nextMatch, buf, pree); // keep trying
				break;
			default:
				Should(false); // no other codes allowed for actions
				return Result::scError;
		}
	} while (res == Result::scMatch);
	return res;
}

Hapy::ostream &Hapy::RuleBase::print(ostream &os) const {
	if (id().known())
		os << id() << " = ";

	if (hasAlg())
		alg().print(os);

	if (Debug(dbdAll)) {
		os << " trim: " << theTrimMode <<
			" auto:" << isImplicit <<
			" leaf:" << isLeaf <<
			" trimmer: " << (void*)theTrimmer <<
			" term: " << (isLeaf || (hasAlg() && alg().terminal())) <<
			" comp: " << isCompiling;
			;
	}
	return os;
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::call(Buffer &buf, Pree &pree, AlgMethod m, const char *mLabel) const {
	const int callId = ++TheLastCallId;

	if (Debug(dbdUser))
		debugTry(buf, pree, mLabel, callId);

	StatusCode res = ((&alg())->*m)(buf, pree);

	if (res == Result::scMatch) {
		pree.match.size(buf.allContent(), 
			buf.parsedSize() - pree.match.start());

		// enforce end-of-file condition if configured to do so
		while (res == Result::scMatch && mustReachEnd && !buf.empty())
			res = HapyAlgCall(nextMatch, buf, pree);
	}

	// apply action, if any; it may change the status
	if (res == Result::scMatch && theAction)
		res = applyAction(buf, pree);

	if (res == Result::scMatch) {
		if (theCommitMode == RuleBase::cmCommit)
			pree.commit();
	}

	if (Debug(dbdUser))
		debugResult(buf, pree, mLabel, callId, res);

	return res;
}

Hapy::ostream &Hapy::RuleBase::debugPfx(int callId) const {
	clog << callId << '/' << TheCallLevel << '-';
	clog << std::setw(2*TheCallLevel) << " ";
	return clog;
}

void Hapy::RuleBase::debugBuffer(const Buffer &buf) const {
	const Buffer::size_type maxSize = 45;
	clog << "buf: ";
	buf.print(clog, maxSize);
	clog << " clen:" << buf.contentSize();
}

void Hapy::RuleBase::debugTry(const Buffer &buf, const Pree &pree, const char *mLabel, int callId) const {
	++TheCallLevel;

	debugPfx(callId) << "try: " << id() << "::" << mLabel << ' ';
	debugBuffer(buf);
	clog << endl;

	if (Debug(dbdAll)) {
		debugPfx(callId) << this <<
			" left: " << buf.parsedSize() <<
			" pree: " << pree.match.start() << ", " << pree.match.size() <<
				" / " << pree.rawCount() <<
			" right: " << buf.contentSize() <<
			" end: " << buf.sawEnd() <<
			endl;
	}
}

void Hapy::RuleBase::debugResult(const Buffer &buf, const Pree &pree, const char *mLabel, int callId, StatusCode status) const {
	debugPfx(callId) << status.str() << ": " << id() << ' ';
	debugBuffer(buf);
	clog << endl;

	if (Debug(dbdAll)) {
		debugPfx(callId) << this << ' ' << status.sc() <<
			" left: " << buf.parsedSize() <<
			" pree: " << pree.match.start() << ", " << pree.match.size() <<
				" / " << pree.rawCount() <<
			" right: " << buf.contentSize() <<
			" end: " << buf.sawEnd() <<
			endl;
	}

	--TheCallLevel;
}

