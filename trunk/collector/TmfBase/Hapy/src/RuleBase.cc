/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Buffer.h>
#include <Hapy/Parser.h>
#include <Hapy/Algorithms.h>
#include <Hapy/RuleBase.h>
#include <Hapy/Debugger.h>
#include <Hapy/Optimizer.h>
#include <Hapy/IoStream.h>
#include <iomanip>
#include <functional>
#include <algorithm>

using Hapy::Debugger::Level;

// these static members are used for debugging only
int Hapy::RuleBase::TheLastCallId = 0;
int Hapy::RuleBase::TheCallLevel = 0;


#ifndef HapyAlgCall
#define HapyAlgCall(alg, buf, pree) \
	call((buf), (pree), &Algorithm::alg, #alg);
#endif

Hapy::RuleBase::RuleBase(): theAlg(0), theTrimmer(0),
	theEmptyMatchState(emsUnknown), theFirstSetState(fssUnknown),
	theSubrules(-1),
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

void Hapy::RuleBase::implicit(bool be) {
	isImplicit = be;
}

bool Hapy::RuleBase::compileTrim(const CFlags &cflags) {
	Should(cflags.trimmer && (cflags.trimLeft || cflags.trimRight));

	if (Debugger::On(Debugger::dbgAll))
		clog << this << " compiling trim: " << cflags.trimmer << " L: " << cflags.trimLeft << " R: " << cflags.trimRight << endl;

	CFlags flags(cflags); // cflags for the next level
	if (theTrimMode == RuleBase::tmVerbatim)
		flags.disableTrim(); // stop trimming at our level
	else
		flags.disableSideTrim(); // do not trim the same place twice

	// prep core
	RulePtr core(new RuleBase(*this));
	core->isCompiling = false;
	core->theTrimmer = 0;
	core->theTrimMode = tmImplicit;
	if (!core->compile(flags))
		return false;

	// the core will be trimmed if needed, disable trim for others
	flags.disableTrim();

	// prep trimmer
	RulePtr trimmer = cflags.trimmer;
	if (trimmer->theCommitMode == RuleBase::cmDefault)
		trimmer->committed(true);
	trimmer->leaf(true);
	trimmer->implicit(true);
	trimmer->theTrimmer = 0;
	if (!trimmer->compile(flags))
		return false;

	// prep out new algorithm combining trim and core rules
	theId = RuleId::Next();
	theId.name(core->id().name() + "_trimmer");
	implicit(true);
	leaf(false);      // but core rule will be if we were
	committed(false); // but core rule will be committed if we were
	theAction.clear();    // but core rule will have it if we had
	theTrimmer = 0;
	theAlg = 0;

	SeqAlg *s = new SeqAlg;
	if (cflags.trimLeft)
		s->add(trimmer);
	s->add(core);
	if (cflags.trimRight)
		s->add(trimmer);
	alg(s);

	return theAlg->compile(flags);
}

bool Hapy::RuleBase::compile(const CFlags &cflags) {
	// prevent infinite recursion
	if (isCompiling) 
		return true;
	isCompiling = true;

	if (Debugger::On(Debugger::dbgAll))
		print(clog << this << " pre rule: " << ' ') << endl;

	if (!Should(theAlg))
		return false;

	CFlags flags(cflags);
	if (shouldTrim(flags)) {
		if (!compileTrim(flags))
			return false;
	} else {
		if (Debugger::On(Debugger::dbgAll))
			clog << this << " will not compile trim" << endl;
		if (theTrimMode == RuleBase::tmVerbatim)
			flags.disableTrim(); // stop trimming at our level
		if (!theAlg->compile(flags))
			return false;
	}

	if (Debugger::On(Debugger::dbgUser)) {
		if (Debugger::On(Debugger::dbgAll))
			clog << this << ' ';
		print(clog << "compiled rule: ") << endl;
	}

	return true;
}

bool Hapy::RuleBase::build(const CFlags &cflags) {
	CFlags flags(cflags);
	flags.reachEnd = false;

	if (!compile(flags))
		return false;

	mustReachEnd = cflags.reachEnd;

	if (false && Optimizer::On()) {
		calcFullFirst();
		Should(theFirstSetState == fssKnown);
	}

	CRules rules;
	collectRules(rules);

	if (!calcMinSize(rules))
		return false;

	if (Debugger::On(Debugger::dbgUser)) {
		DeepPrinted printed;
		deepPrint(clog << "built " << rules.size() << "-rule grammar:" << endl,
			printed);
	}

	return true;
}

int Hapy::RuleBase::subrules() const {
	Assert(theSubrules >= 0);
	return theSubrules;
}

void Hapy::RuleBase::collectRules(CRules &rules) {
	// this check prevents infinite recursion
	if (find(rules.begin(), rules.end(), this) == rules.end()) {
		rules.push_back(this);
		const CRules::size_type wasRules = rules.size();
		theAlg->collectRules(rules);
		theSubrules = rules.size() - wasRules; // XXX: others add to rules too
	}
}

bool Hapy::RuleBase::calcMinSize(CRules &rules) {

	if (Debugger::On(Debugger::dbgUser))
		clog << "computing " << rules.size() << " rule sizes" << endl;

	typedef CRules::iterator CRI;

	for (CRI i = rules.begin(); i != rules.end(); ++i) {
		RulePtr r = *i;

		if (r->theSizeCalc.withValue())
			continue;

		if (Debugger::On(Debugger::dbgAll))
			r->print(clog << " calcMinSize starts for: ") << endl;

		SizeCalcPass pass(r);
		const SizeCalcLocal::size_type size = r->calcMinSize(pass);
		if (!Should(size != SizeCalcLocal::nsize))
			return false;

		r->theSizeCalc.value(size);

		if (Debugger::On(Debugger::dbgAll))
			r->print(clog << " calcMinSize done for: ") << endl;
	}

	if (Debugger::On(Debugger::dbgUser))
		clog << "computed all " << rules.size() << " rule sizes" << endl;

	return true;
}

// may be called recursively
Hapy::SizeCalcLocal::size_type Hapy::RuleBase::calcMinSize(SizeCalcPass &pass) {
	if (theSizeCalc.withValue())
		return theSizeCalc.value();

	if (theSizeCalc.doing(pass)) { // already doing this pass?
		if (pass.minLoopDepth < 0 || theSizeCalc.depth() < pass.minLoopDepth)
			pass.minLoopDepth = theSizeCalc.depth();
		return SizeCalcLocal::nsize;
	}

	const int wasMinLoopDepth = pass.minLoopDepth;
	pass.minLoopDepth = -1; // reset to get a fresh reading

	theSizeCalc.start(pass);
	if (Debugger::On(Debugger::dbgAll))
		print(clog << this << " calcMinSize down(" << pass.depth << "): ") << endl;
	const SizeCalcLocal::size_type size = theAlg->calcMinSize(pass);
	theSizeCalc.stop(pass);

	const bool lowerDepthLoops = pass.minLoopDepth >= 0 &&
		pass.minLoopDepth < theSizeCalc.depth();
	if (!lowerDepthLoops) { // no higher level loops
		if (Should(size != SizeCalcLocal::nsize))
			theSizeCalc.value(size);
	}		

	if (Debugger::On(Debugger::dbgAll))
		print(clog << this << " calcMinSize up(" << pass.depth+1 << "):   ") << " msize: " << size << " loops: " << lowerDepthLoops << ':' << pass.minLoopDepth << '/' << theSizeCalc.depth() << endl;

	// restore minLoopDepth for upper rules
	if (wasMinLoopDepth  >= 0 &&
		(pass.minLoopDepth < 0 || wasMinLoopDepth < pass.minLoopDepth))
		pass.minLoopDepth = wasMinLoopDepth;

	return size;
}

Hapy::SizeCalcLocal::size_type Hapy::RuleBase::minSize() const {
	Assert(theSizeCalc.withValue());
	return theSizeCalc.value();
}

bool Hapy::RuleBase::shouldTrim(CFlags &flags) const {
	if (theTrimmer) {
		flags.enableTrim(theTrimmer);
		if (Debugger::On(Debugger::dbgAll))
			clog << this << " enabling trimmer: " << flags.trimmer << " L: " << flags.trimLeft << " R: " << flags.trimRight << endl;
	} else {
		if (Debugger::On(Debugger::dbgAll))
			clog << this << " import trimmer:   " << flags.trimmer << " L: " << flags.trimLeft << " R: " << flags.trimRight << endl;
		if (!flags.trimmer)
			return false;
		if (!Should(theTrimMode != RuleBase::tmExplicit)) // paranoid
			return false;
		if (!flags.trimLeft && !flags.trimRight)
			return false; // but may still need to propagate trimming down
	}
	return true;
}

bool Hapy::RuleBase::calcPartialFirst(First &first, Pree &pree) {
	if (Debugger::On(Debugger::dbgAll))
		print(clog << this << " calcPartialFirst called:   " << ' ') << " state: " << theFirstSetState << endl;

	Should(isCompiling);

	pree.rawRid(id());
	pree.match.start(0);

	switch (theFirstSetState) {
		case fssKnown:
			first = theFirst;
			return true;
		case fssComputing:
			return false;
		case fssError:
			return false;
		default: {
			return theAlg->calcPartialFirst(first, pree);
		}
	}

	return Should(false);
}

void Hapy::RuleBase::calcFullFirst() {
	if (Debugger::On(Debugger::dbgAll))
		print(clog << this << " calcFullFirst called:   " << ' ') << " state: " << theFirstSetState << endl;

	if (theFirstSetState == fssUnknown) {
		theFirstSetState = fssComputing;

		if (Debugger::On(Debugger::dbgAll))
			print(clog << this << " before calculating full first:   " << ' ') << " state: " << theFirstSetState << endl;

		theAlg->calcFullFirst();

		if (Debugger::On(Debugger::dbgAll))
			print(clog << this << " after calculating full first:   " << ' ') << " state: " << theFirstSetState << endl;

		Pree pree;
		// do not call this->calcPartialFirst() here because it updates rules
		theFirstSetState = theAlg->calcPartialFirst(theFirst, pree) ?
			fssKnown : fssError;

		if (Debugger::On(Debugger::dbgAll)) {
			const First *f = (theFirstSetState == fssKnown) ? &theFirst : 0;
			print(clog << this << " calculated FIRST: " << ' ') << f << " state: " << theFirstSetState << endl;
			if (f)
				clog << (f->hasEmpty() ? "\twith empty" : "without empty") << endl;
			for (int i = 0; f && i <= 255; ++i) {
				if (f->has(i))
					clog << "\tset[" << std::setw(3) << i << "]: " << (char)i << endl;
			}
		}
	}
}

bool Hapy::RuleBase::temporary() const {
	return id().temporary();
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::firstMatch(Buffer &buf, Pree &pree) const {
	if (!mayMatch(buf))
		return Result::scMiss;

	pree.rawRid(id());
	pree.match.start(buf.parsedSize());
	pree.implicit = isImplicit;
	pree.leaf = isLeaf;

	// check whether remaining content "fits" into expected tree
	// this check handles left recursion cases that increase minSize
	const SizeCalcLocal::size_type rightSize = pree.expectedMinSize();
	const SizeCalcLocal::size_type hereSize = minSize();
	const SizeCalcLocal::size_type neededSize = hereSize + rightSize;

	if (neededSize > buf.contentSize()) {
		DebugReject(this, "min size exceeds remaining input size");
		return Result::scMiss;
	}

	// check for pointless and endless recursion, like rA = rA
	if (pree.emptyVerticalLoop()) {
		DebugReject(this, "pointless left recursion");
		return Result::scMiss;
	}

	return HapyAlgCall(firstMatch, buf, pree);
}

// note: HapyAlgCall() and applyAction() may bypass this interface
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

// XXX: check that algorithms do not return scMore when buf.sawEnd() ?
// because some algorithms may act differently depending on sawEnd, we should
// let each algorithm see sawEnd state and decide
bool Hapy::RuleBase::mayMatch(Buffer &buf) const {
	if (true || !Optimizer::On())
		return true;

	if (theFirstSetState != fssKnown) {
		Should(theFirstSetState == fssError);
		return true; // disables FIRST optimization if things are broken
	}

	if (Debugger::On(Debugger::dbgAll)) {
		clog << "FIRST: " << 
			(theFirst.hasEmpty() ? "with empty" : "without empty") <<
			"; state: " << theFirstSetState << endl;
		if (!buf.empty())
			clog << "\tpeek: " << buf.peek() << " in " << &theFirst << endl;
		for (int i = 0; i <= 255; ++i) {
			if (theFirst.has(i))
				clog << "\tset[" << std::setw(3) << i << "]: " << (char)i << endl;
		}
	}

	if (theFirst.hasEmpty())
		return true;

	if (buf.empty()) {
		// we cannot return false/scMore if did not see end because
		// the alg must adjust pree to be able to resume;
		// perhaps that is an alg interface flaw?
		if (!buf.sawEnd())
			return true;
	} else
	if (theFirst.has(buf.peek()))
		return true;

	if (Debugger::On(Debugger::dbgUser))
		DebugReject(this, "FIRST mismatch");

	return false;
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

void Hapy::RuleBase::action(const Action &anAction) {
	theAction = anAction; // OK to overwrite
}

void Hapy::RuleBase::updateAlg(const RuleBase &src) {
	if (Should(src.hasAlg()))
		theAlg = src.theAlg;
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::applyAction(Buffer &buf, Pree &pree) const {
	StatusCode res = Result::scMatch;
	do {
		ActionBase::Params params(buf, pree, res);
		theAction.base->act(&params);
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

	if (Debugger::On(Debugger::dbgAll)) {
		os << " [" << this;
		if (theTrimMode)
			os << " trim: " << theTrimMode;
		if (theCommitMode)
			os << " comm: " << theCommitMode;
		if (theSubrules > 0)
			os << " sub: " << theSubrules;
		if (theSizeCalc.withValue())
			os << " msz: " << theSizeCalc.value();
		if (theTrimmer)
			os << " trimmer: " << (void*)theTrimmer;
		if (isImplicit)
			os << " auto";
		if (isLeaf)
			os << " leaf";
		if (isLeaf || (hasAlg() && alg().terminal()))
			os << " term";
		if (!isCompiling)
			os << " !comp";
		os << "]";
	}
	return os;
}

void Hapy::RuleBase::deepPrint(ostream &os, DeepPrinted &printed) const {
	if (printed.find(this) == printed.end()) { // print only once
		printed.insert(this);
		this->print(os << '\t') << endl;
		theAlg->deepPrint(os, printed);
	}
}

Hapy::RuleBase::StatusCode Hapy::RuleBase::call(Buffer &buf, Pree &pree, AlgMethod m, const char *mLabel) const {
	const int callId = ++TheLastCallId;

	if (Debugger::On(Debugger::dbgUser))
		debugTry(buf, pree, mLabel, callId);

	StatusCode res = ((&alg())->*m)(buf, pree);

	if (res == Result::scMatch) {
		pree.match.size(buf.allContent(), 
			buf.parsedSize() - pree.match.start());

		// enforce end-of-file condition if configured to do so
		while (res == Result::scMatch && mustReachEnd && !pree.up && !buf.empty())
			res = HapyAlgCall(nextMatch, buf, pree);
	}

	// apply action, if any; it may change the status
	if (res == Result::scMatch && theAction)
		res = applyAction(buf, pree);

	if (res == Result::scMatch && Optimizer::On()) {
		if (theCommitMode == RuleBase::cmCommit)
			pree.commit();
	}

	if (Debugger::On(Debugger::dbgUser))
		debugResult(buf, pree, mLabel, callId, res);

	return res;
}

Hapy::ostream &Hapy::RuleBase::DebugPfx(int callId) {
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

void Hapy::RuleBase::DebugReject(const RuleBase *rule, const char *reason) {
	if (Debugger::On(Debugger::dbgUser)) {
		++TheCallLevel;
	
		DebugPfx(++TheLastCallId) << "reject: " << rule->id() << 
			" reason: " << reason;

		if (Debugger::On(Debugger::dbgAll))
			clog << ' ' << rule;

		clog << endl;

		--TheCallLevel;
	}
}

void Hapy::RuleBase::debugTry(const Buffer &buf, const Pree &pree, const char *mLabel, int callId) const {
	++TheCallLevel;

	DebugPfx(callId) << "try: " << id() << "::" << mLabel << ' ';
	debugBuffer(buf);
	clog << endl;

	if (Debugger::On(Debugger::dbgAll)) {
		DebugPfx(callId) << this <<
			" left: " << buf.parsedSize() <<
			" pree: " << pree.match.start() << ", " << pree.match.size() <<
				" / " << pree.rawCount() <<
				" < " << pree.minSize <<
			" right: " << buf.contentSize() <<
			" end: " << buf.sawEnd() <<
			endl;
	}
}

void Hapy::RuleBase::debugResult(const Buffer &buf, const Pree &pree, const char *mLabel, int callId, StatusCode status) const {
	DebugPfx(callId) << status.str() << ": " << id() << ' ';
	debugBuffer(buf);
	clog << endl;

	if (Debugger::On(Debugger::dbgAll)) {
		DebugPfx(callId) << this << ' ' << status.sc() <<
			" left: " << buf.parsedSize() <<
			" pree: " << pree.match.start() << ", " << pree.match.size() <<
				" / " << pree.rawCount() <<
				" < " << pree.minSize <<
			" right: " << buf.contentSize() <<
			" end: " << buf.sawEnd() <<
			endl;
	}

	--TheCallLevel;
}

