/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Algorithms.h>
#include <Hapy/Rule.h>
#include <Hapy/Rules.h>
#include <Hapy/RuleBase.h>
#include <Hapy/IoStream.h>


Hapy::Rule::Rule(): theBase(new RuleBase) {
	base()->id(RuleId::Next());
}

Hapy::Rule::Rule(const RulePtr &aBase): theBase(aBase) {
}

Hapy::Rule::Rule(const string &aName, RuleId *id): theBase(new RuleBase) {
	base()->id(RuleId::Next());
	base()->id().name(aName);
	if (id)
		*id = base()->id();
}

Hapy::Rule::Rule(const Rule &r): theBase(r.theBase) {
}

Hapy::Rule::Rule(const string &s): theBase(string_r(s).theBase) {
}

Hapy::Rule::Rule(const char *s): theBase(string_r(s).theBase) {
}

Hapy::Rule::Rule(char c): theBase(char_r(c).theBase) {
}

Hapy::Rule::Rule(int): theBase(0) {
	Assert(false); // should no be called
}

Hapy::Rule &Hapy::Rule::operator =(const Rule &r) {
	if (&r == this)
		return *this; // assignment to self

	if (base()->temporary()) {
		Should(!base()->hasAlg() || !r.base()->hasAlg());
		theBase = r.theBase;
	} else
	if (r.base()->temporary()) {
		base()->updateAlg(*r.base());
		// other parts of base are not updated
	} else {
		ProxyAlg *pr = new ProxyAlg(r.base()); // XXX: delete; leaking algs
		base()->alg(pr);
		// theBase is not updated but new base is used via proxy
	}
	return *this;
}

Hapy::Rule::~Rule() {
}

const Hapy::RuleId &Hapy::Rule::id() const {
	return theBase->id();
}

void Hapy::Rule::committed(bool be) {
	base()->committed(be);
}

void Hapy::Rule::trim(const Rule &r) {
	base()->trim(r.base());
}

// remember explicit no-internal-trimming request
void Hapy::Rule::verbatim(bool be) {
	base()->verbatim(be);
}

// remember leaf setting
void Hapy::Rule::leaf(bool be) {
	base()->leaf(be);
}

void Hapy::Rule::action(const Action *a) {
	base()->action(a);
}

void Hapy::Rule::ptr_action(ActionPtr p) {
	action(Hapy::ptr_action(p));
}

Hapy::ostream &Hapy::Rule::print(ostream &os) const {
	if (id().known())
		os << id() << " = ";
	if (base()->hasAlg())
		base()->alg().print(os);
	return os;
}

void Hapy::Rule::Debug(bool doIt) {
	RuleBase::TheDebugDetail = doIt ? RuleBase::dbdUser : RuleBase::dbdNone;
}

