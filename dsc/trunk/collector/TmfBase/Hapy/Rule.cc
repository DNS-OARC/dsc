 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include <functional>
#include <algorithm>
#include "xstd/Assert.h"
#include "xstd/ForeignMemFun.h"
#include "xstd/Bind2Ref.h"

#include "Hapy/Buffer.h"
#include "Hapy/Parser.h"
#include "Hapy/RuleAlgs.h"
#include "Hapy/Rule.h"

using namespace Hapy;


/* RuleBase */

Hapy::RuleBase::RuleBase(): alg(0), id(0), core(0), itrimmer(0),
	commitMode(cmDefault), trimMode(tmDefault), isLeaf(false),
	compiling(false) {
}


/* RuleBasePtr */

Hapy::RuleBasePtr::RuleBasePtr(): thePtr(new RuleBase) {
}

Hapy::RuleBasePtr::RuleBasePtr(const RuleBasePtr &p): thePtr(p.thePtr) {
}

Hapy::RuleBasePtr::~RuleBasePtr() {
	// delete thePtr; // XXX: leaking links, need refcounting
	thePtr = 0;
}

RuleBasePtr &Hapy::RuleBasePtr::operator =(const RuleBasePtr &p) {
	// nothing special going on here; added for completeness
	if (&p == this)
		return *this; // assignment to self
	thePtr = p.thePtr;
	return *this;
}


/* Rule */

#define debug(label, buf, pree) ;

#define dodebug(label, buf, pree) \
	cerr << here << this << ' '<< name() << "::" << label << \
		" raw_cnt: " << pree.rawCount() << \
		" trim: " << pree.trimming << \
		" buf: " << buf.content().substr(0,20) << endl;

Hapy::Rule::Rule() {
}

Hapy::Rule::Rule(const string &aName, int anId) {
	theBase->name = aName;
	theBase->id = anId;
}

Hapy::Rule::Rule(const Rule &r): theBase(r.theBase) {
}

Hapy::Rule::Rule(const string &s): theBase(string_r(s).theBase) {
}

Hapy::Rule::Rule(const char *s): theBase(string_r(s).theBase) {
}

Rule &Hapy::Rule::operator =(const Rule &r) {
	if (&r == this)
		return *this; // assignment to self

	// assignments update user-accessible/visible core, not trimming details
	if (theBase->core)
		return *theBase->core = r;

	if (this->temporary()) {
		Should(!hasAlg() || !r.hasAlg());
		theBase = r.theBase;
	} else
	if (r.temporary()) {
		if (Should(r.hasAlg()))
			alg(r.theBase->alg);
		// theBase is not updated
	} else {
		ProxyRule *pr = new ProxyRule(r); // XXX: delete; leaking algs
		alg(pr);
		// theBase is not updated but new base is used via proxy
	}
	return *this;
}

Hapy::Rule::~Rule() {
}

void Hapy::Rule::committed(bool be) {
	theBase->commitMode = be ? RuleBase::cmCommit : RuleBase::cmDont;
}

bool Hapy::Rule::terminal() const {
	Should(theBase->compiling);
	return theBase->isLeaf ||
		(Should(theBase->compiling) && hasAlg() && alg().terminal());
}

bool Hapy::Rule::trimming() const {
	return theBase->core != 0;
}

// remember explicit trimming request
void Hapy::Rule::trim(const Rule &skipper) {
	Should(!theBase->itrimmer);
	theBase->itrimmer = new Rule(skipper); // XXX: leaking trimmers
	theBase->trimMode = RuleBase::tmExplicit;
}

// remember explicit no-internal-trimming request
void Hapy::Rule::verbatim(bool be) {
	theBase->trimMode = RuleBase::tmVerbatim;
}

// remember leaf setting
void Hapy::Rule::leaf(bool be) {
	theBase->isLeaf = be;
}

// set trimming if no trimming was configured
void Hapy::Rule::implicitTrim(const Rule &skipper) {
	if (theBase->trimMode == RuleBase::tmDefault) {
		Should(theBase->itrimmer == 0);
		theBase->trimMode = RuleBase::tmImplicit;
	} else
	if (theBase->trimMode == RuleBase::tmVerbatim) {
		if (theBase->itrimmer)
			return; // XXX: assume this is the same trimming rule
	} else {
		return;
	}

	theBase->itrimmer = new Rule(skipper); // XXX: leaking trimmers
}

bool Hapy::Rule::compileTrim() {
	if (!Should(theBase->core == 0))
		return false;

	if (!Should(theBase->itrimmer))
		return false;

	Rule *trimmer = theBase->itrimmer;
	if (trimmer->theBase->commitMode == RuleBase::cmDefault)
		trimmer->committed(true);

	Rule *c = new Rule;
	*c->theBase = *this->theBase;
	theBase->id = 0;
	theBase->name += "_trimming";
	theBase->itrimmer = 0;
	committed(false); // but core rule will be committed if we were

	theBase->alg = 0;
	theBase->core = c;
	SeqRule *s = new SeqRule;
	s->add(*trimmer);
	s->add(*c);
	s->add(*trimmer);
	alg(s);

	return true;
}

bool Hapy::Rule::compile() {
	// prevent infinite recursion
	if (theBase->compiling) 
		return true;
	theBase->compiling = true;

#if HAPY_DEBUG
print(cerr << here << "pre rule:  ") << endl;
#endif

	if (!Should(theBase->alg))
		return false;

	if (theBase->itrimmer && theBase->trimMode != RuleBase::tmVerbatim)
		theBase->alg->spreadTrim(*theBase->itrimmer);

	if (!theBase->alg->compile())
		return false;

	if (theBase->itrimmer && terminal() && !compileTrim())
		return false;

#if HAPY_DEBUG
print(cerr << here << "comp rule: ") << endl;
if (theBase->core)
theBase->core->print(cerr << here << "comp core: ") << endl;
#endif
	return true;
}

bool Hapy::Rule::temporary() const {
	return name().empty() && id() <= 0;
}

Rule::StatusCode Hapy::Rule::firstMatch(Buffer &buf, PreeNode &pree) const {
	pree.rawRid(id());
	pree.trimming = this->trimming();
	pree.leaf = theBase->isLeaf;
	debug("firstMatch", buf, pree);
	return alg().firstMatch(buf, pree);
}

Rule::StatusCode Hapy::Rule::nextMatch(Buffer &buf, PreeNode &pree) const {
	Should(pree.rawRid() == id());
	debug("nextMatch", buf, pree);
	if (theBase->commitMode == RuleBase::cmCommit) {
		cancel(buf, pree);
		return Result::scMiss;
	}
	return alg().nextMatch(buf, pree);
}

Rule::StatusCode Hapy::Rule::resume(Buffer &buf, PreeNode &pree) const {
	Should(pree.rawRid() == id());
	debug("resume", buf, pree);
	return alg().resume(buf, pree);
}

void Hapy::Rule::cancel(Buffer &buf, PreeNode &pree) const {
	Should(pree.rawRid() == id());
	debug("cancel", buf, pree);
	alg().cancel(buf, pree);
}

const RuleAlg &Hapy::Rule::alg() const {
	Assert(theBase->alg);
	return *theBase->alg;
}

void Hapy::Rule::alg(RuleAlg *anAlg) {
	Assert(anAlg);
	Assert(!theBase->alg); // assume rule redefinitions are bugs
	theBase->alg = anAlg;
}

ostream &Hapy::Rule::print(ostream &os) const {
	if (name().size())
		os << name() << '(' << id() << ')' << " = ";
	if (hasAlg())
		alg().print(os);
	os << " trim: " << theBase->trimMode <<
		" leaf:" << theBase->isLeaf <<
		" trimmer: " << (void*)theBase->itrimmer <<
		" term: " << (theBase->isLeaf || (hasAlg() && alg().terminal())) <<
		" comp: " << theBase->compiling;
		;
	return os;
}
