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


/* RuleAlgPtr */

Hapy::RuleAlgPtr::Link::Link(): alg(0), next(0) {
}

const RuleAlgPtr::Link *Hapy::RuleAlgPtr::Link::find() const {
	// XXX: optimize: cache found link

	if (alg)
		return this;
	
	if (next)
		return next->find();

	return 0;
}

Hapy::RuleAlgPtr::RuleAlgPtr(): theLink(new Link) {
}

// cannot assign p.Link to theLink because of repoint()
Hapy::RuleAlgPtr::RuleAlgPtr(const RuleAlgPtr &p): theLink(new Link) {
	theLink->next = p.theLink;
}

Hapy::RuleAlgPtr::~RuleAlgPtr() {
	// delete theLink; // XXX: leaking links, need refcounting
	theLink = 0;
}

RuleAlgPtr &Hapy::RuleAlgPtr::operator =(const RuleAlgPtr &p) {
	if (&p == this)
		return *this; // assignment to self

	Assert(theLink);
	Assert(p.theLink);
	Assert(!theLink->alg);  // user does not accidently redefine known rule
	Assert(!theLink->next); // user does not accidently redefine future rule
	theLink->next = p.theLink;
	return *this;
}

bool Hapy::RuleAlgPtr::known() const {
	return theLink && theLink->find() != 0;
}

const RuleAlg &Hapy::RuleAlgPtr::value() const {
	const Link *link = theLink->find();
	Assert(link);
	Assert(link->alg);
	return *link->alg;
}

RuleAlg &Hapy::RuleAlgPtr::value() {
	const Link *link = theLink->find();
	Assert(link);
	Assert(link->alg);
	return *link->alg;
}

void Hapy::RuleAlgPtr::value(RuleAlg *anAlg) {
	Assert(anAlg);
	Assert(theLink);
	Assert(!theLink->next);
	Assert(!theLink->alg);
	theLink->alg = anAlg;
}

void Hapy::RuleAlgPtr::repoint(RuleAlg *anAlg) {
	Assert(anAlg);
	Assert(theLink);

	// we cannot delete/new theLink because others may point to it
	Link *newPoint = new Link; // XXX: never deleted; leaking links
	newPoint->alg = anAlg;

	// theCore knows what we were pointing to; others get new alg
	theLink->next = newPoint;  
	theLink->alg = 0;
}


/* Rule */

#if CERR_DEBUG
#define debug(label, buf, pree) \
	cerr << here << this << ' '<< theName << "::" << label << \
		" raw_cnt: " << pree.rawCount() << \
		" trim: " << pree.trimming << \
		" buf: " << buf.content().substr(0,20) << endl;
#else
#define debug(label, buf, pree) (void) 0;
#endif

Hapy::Rule::Rule(): theId(0), theCore(0), theCommitMode(cmDefault) {
}

Hapy::Rule::Rule(const string &aName, int anId): theName(aName), theId(anId),
	theCore(0), theCommitMode(cmDefault) {
}

Hapy::Rule::Rule(const Rule &r):  
	theId(0), theCore(0), theCommitMode(cmDefault) {
	init(r);
}

Hapy::Rule::Rule(const string &s): theId(0), theCore(0) {
	init(string_r(s));
}

Hapy::Rule::Rule(const char *s): theId(0), theCore(0) {
	Should(s);
	init(string_r(s));
}

void Hapy::Rule::init(const Rule &r) {
	Should(!theId && !theCore); // init is called from constructors
	theId = r.theId;
	theName = r.theName;
	theAlg = r.theAlg;
	// theCore is not updated (but the alg is, above)
	theCommitMode = r.theCommitMode;
}

Rule &Hapy::Rule::operator =(const Rule &r) {
	if (&r == this)
		return *this; // assignment to self

	// assignments update user-accessible/visible core, not trimming details
	if (theCore)
		return *theCore = r;

	ProxyRule *pr = new ProxyRule(r); // XXX: delete; leaking algs
	alg(pr);
	// theCore and theCommitMode is not updated but used via Proxy
	return *this;
}

Hapy::Rule::~Rule() {
	// delete theCore; // XXX: leaking if trimming
}

void Hapy::Rule::committed(bool be) {
	theCommitMode = be ? cmCommit : cmDont;
}

bool Hapy::Rule::trimming() const {
	return theCore != 0;
}

void Hapy::Rule::trim(const Rule &skipper) {
	Should(!trimming());

	Rule trimmer(skipper);
	Should(!trimmer.trimming());
	if (trimmer.theCommitMode == cmDefault)
		trimmer.committed(true);

	theCore = new Rule(theName, theId);
	theCore->alg(&theAlg.value());
	theCore->theCommitMode = this->theCommitMode;
	this->theName += "_trimmed";
	this->committed(false); // but core rule will be committed if we were

	SeqRule *s = new SeqRule;
	s->add(trimmer);
	s->add(*theCore);
	s->add(trimmer);
	theAlg.repoint(s);
print(cerr << here << "repointed: ") << endl;
}

bool Hapy::Rule::anonymous() const {
	return theName.empty() && theId <= 0;
}

Rule::StatusCode Hapy::Rule::firstMatch(Buffer &buf, PreeNode &pree) const {
	pree.rid = theId;
	pree.trimming = this->trimming();
	debug("firstMatch", buf, pree);
	return alg().firstMatch(buf, pree);
}

Rule::StatusCode Hapy::Rule::nextMatch(Buffer &buf, PreeNode &pree) const {
	Should(pree.rid == theId);
	debug("nextMatch", buf, pree);
	if (theCommitMode == cmCommit) {
		cancel(buf, pree);
		return Result::scMiss;
	}
	return alg().nextMatch(buf, pree);
}

Rule::StatusCode Hapy::Rule::resume(Buffer &buf, PreeNode &pree) const {
	Should(pree.rid == theId);
	debug("resume", buf, pree);
	return alg().resume(buf, pree);
}

void Hapy::Rule::cancel(Buffer &buf, PreeNode &pree) const {
	Should(pree.rid == theId);
	debug("cancel", buf, pree);
	alg().cancel(buf, pree);
}

const RuleAlg &Hapy::Rule::alg() const {
	return theAlg.value();
}

void Hapy::Rule::alg(RuleAlg *anAlg) {
	theAlg.value(anAlg);
}

ostream &Hapy::Rule::print(ostream &os) const {
	if (name().size())
		os << name() << '(' << id() << ')' << " = ";
	if (hasAlg())
		alg().print(os);
	return os;
}
