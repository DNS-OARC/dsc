/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Algorithms.h>
#include <Hapy/Rule.h>
#include <Hapy/Rules.h>
#include <Hapy/RuleBase.h>
#include <Hapy/IoStream.h>


// the only constructor that uses the base without assigning it
// used for internal purposes such as creation of rules from algorithms
Hapy::Rule::Rule(const RulePtr &aBase): theBase(aBase) {
}

Hapy::Rule::Rule(): theBase(0) {
	init(RuleId::Next());
}

Hapy::Rule::Rule(const string &aName, RuleId *id): theBase(0) {
	init(RuleId::Next());
	base()->id().name(aName);
	if (id)
		*id = base()->id();
}

Hapy::Rule::Rule(const Rule &r): theBase(0) {
	init(RuleId::Next());
	assign(r.base());
}

Hapy::Rule::Rule(const string &s): theBase(0) {
	init(RuleId::Temporary());
	assign(string_r(s).theBase);
}

Hapy::Rule::Rule(const char *s): theBase(0) {
	init(RuleId::Temporary());
	assign(string_r(s).theBase);
}

Hapy::Rule::Rule(char c): theBase(0) {
	init(RuleId::Temporary());
	assign(char_r(c).theBase);
}

Hapy::Rule::Rule(int): theBase(0) {
	Assert(false); // should no be called
}

Hapy::Rule::~Rule() {
}

void Hapy::Rule::init(const RuleId &rid) {
	Should(!theBase);
	theBase = new RuleBase;
	base()->id(rid);
}

void Hapy::Rule::assign(RuleBase *b) {
	Assert(b);
	if (b == base()) // check for assignment to self
		return;

	if (b->temporary()) {
		// we do not use a proxy algorithm because the user does not
		// expect an extra level in the parsing tree where no explicit
		// rule exists
		base()->updateAlg(*b);
		id().autoName(b->id().name());
		// other properties of our base are not updated because our settings
		// should overwrite and because temporary rules should not have
		// their own properties
	} else {
		// theBase is not updated but new base is used via proxy
		ProxyAlg *pr = new ProxyAlg(b); // XXX: delete; leaking algs
		base()->alg(pr);
	}
}

Hapy::Rule &Hapy::Rule::operator =(const Rule &r) {
	assign(r.base());
	return *this;
}

bool Hapy::Rule::known() const {
	return theBase && theBase->hasAlg();
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

void Hapy::Rule::action(const Action &a) {
	base()->action(a);
}

Hapy::Rule Hapy::Rule::operator [](const Action &a) const {
	const string oldName = id().name();
	const string newName = oldName.empty() ?
		string("_copy[]") : ("_" + oldName + "[]");

	// our base will share theTrimmer with the clone; XXX: delete it carefully
	RuleBase *clone = new RuleBase(*base());
	clone->id().name(newName);
	clone->action(a);
	return Rule(clone); // will not trigger assign()
}

Hapy::ostream &Hapy::Rule::print(ostream &os) const {
	if (id().known())
		os << id() << " = ";
	if (base()->hasAlg())
		base()->alg().print(os);
	return os;
}
