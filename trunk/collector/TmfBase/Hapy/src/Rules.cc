/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Rules.h>
#include <Hapy/RulePtr.h>
#include <Hapy/RuleBase.h>
#include <Hapy/Algorithms.h>

namespace Hapy {

static
Rule AlgToRule(Algorithm *alg) {
	RulePtr p(new RuleBase);
	p->id(RuleId::Temporary());
	string name;
	if (alg->terminal(&name) && name.size() > 0)
		p->id().autoName(name);

	p->alg(alg);
	return Rule(p);
}

static
bool AlgMergeable(const RulePtr &a, const char *tag) {
	return a->id().temporary() && a->hasAlg() && a->alg().isA(tag);
}

} // namespace Hapy

Hapy::Rule Hapy::operator !(const Rule &a) {
	ReptionAlg *alg = new ReptionAlg(a.base(), 0, 1);
	return AlgToRule(alg);
}

Hapy::Rule Hapy::operator |(const Rule &ra, const Rule &rb) {
	OrAlg *alg = new OrAlg; // XXX: leak; here and below
	const RulePtr &a = ra.base();
	if (AlgMergeable(a, "Or"))
		alg->addMany((const OrAlg&)a->alg());
	else 
		alg->add(a);
	alg->add(rb.base());
	return AlgToRule(alg);
}

Hapy::Rule Hapy::operator -(const Rule &a, const Rule &b) {
	DiffAlg *alg = new DiffAlg(a.base(), b.base());
	return AlgToRule(alg);
}

Hapy::Rule Hapy::operator >>(const Rule &ra, const Rule &rb) {
	SeqAlg *alg = new SeqAlg;
	const RulePtr &a = ra.base();
	if (AlgMergeable(a, "Seq"))
		alg->addMany((const SeqAlg&)a->alg());
	else 
		alg->add(a);
	alg->add(rb.base());
	return AlgToRule(alg);
}

Hapy::Rule Hapy::operator *(const Rule &a) {
	ReptionAlg *alg = new ReptionAlg(a.base(), 0);
	return AlgToRule(alg);
}

Hapy::Rule Hapy::operator +(const Rule &a) {
	ReptionAlg *alg = new ReptionAlg(a.base(), 1);
	return AlgToRule(alg);
}


Hapy::Rule Hapy::char_r(char c) {
	return string_r(string(1, c));
}

Hapy::Rule Hapy::char_set_r(const string &s) {
	return AlgToRule(new SomeCharAlg(s));
}

Hapy::Rule Hapy::char_range_r(char first, char last) {
	return AlgToRule(new CharRangeAlg(first, last));
}

Hapy::Rule Hapy::string_r(const string &s) {
	return AlgToRule(new StringAlg(s));
}

Hapy::Rule Hapy::quoted_r(const Rule &element, const Rule &open, const Rule &close) {
	SeqAlg *s = new SeqAlg;
	s->add(open.base());
	s->add((*(element-close)).base());
	s->add(close.base());
	Rule r(AlgToRule(s));
	r.committed(true);
	//r.verbatim(true);
	return r;
}

Hapy::Rule Hapy::quoted_r(const Rule &element, const Rule &open) {
	return quoted_r(element, open, open);
}

Hapy::Rule Hapy::quoted_r(const Rule &element) {
	return quoted_r(element, char_r('"'));
}

const Hapy::Rule Hapy::anychar_r = Hapy::AlgToRule(new Hapy::AnyCharAlg);

const Hapy::Rule Hapy::alpha_r = Hapy::AlgToRule(new Hapy::AlphaAlg);

const Hapy::Rule Hapy::digit_r = Hapy::AlgToRule(new Hapy::DigitAlg);

const Hapy::Rule Hapy::alnum_r = Hapy::alpha_r | Hapy::digit_r;

const Hapy::Rule Hapy::space_r = Hapy::AlgToRule(new Hapy::SpaceAlg);

const Hapy::Rule Hapy::eol_r = Hapy::string_r("\n"); // for now

const Hapy::Rule Hapy::empty_r = Hapy::AlgToRule(new Hapy::EmptyAlg);

const Hapy::Rule Hapy::end_r = Hapy::AlgToRule(new Hapy::EndAlg);
