 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/Assert.h"

#include "Hapy/Buffer.h"
#include "Hapy/Parser.h"
#include "Hapy/Rules.h"
#include "Hapy/RuleAlgs.h"

using namespace Hapy;

inline
Rule AlgToRule(RuleAlg *alg) {
	Rule r;
	r.alg(alg);
	return r;
}

Rule Hapy::operator !(const Rule &a) {
	return a | empty_r();
}

Rule Hapy::operator |(const Rule &a, const Rule &b) {
	OrRule *alg = new OrRule; // XXX: leak; here and below
	if (a.hasAlg() && a.alg().isA("OrRule"))
		alg->addMany((const OrRule&)a.alg());
	else 
		alg->add(a);
	alg->add(b);
	return AlgToRule(alg);
}

Rule Hapy::operator -(const Rule &a, const Rule &b) {
	DiffRule *alg = new DiffRule(a, b);
	return AlgToRule(alg);
}

Rule Hapy::operator >>(const Rule &a, const Rule &b) {
	SeqRule *alg = new SeqRule;
	if (a.hasAlg() && a.alg().isA("SeqRule"))
		alg->addMany((const SeqRule&)a.alg());
	else 
		alg->add(a);
	alg->add(b);
	return AlgToRule(alg);
}

Rule Hapy::operator *(const Rule &a) {
	StarRule *alg = new StarRule(a);
	return AlgToRule(alg);
}

Rule Hapy::operator +(const Rule &a) {
	SeqRule *alg = new SeqRule;
	alg->add(a);
	alg->add(*a);
	return AlgToRule(alg);
}


Rule Hapy::char_r(char c) {
	return string_r(string(1, c));
}

Rule Hapy::string_r(const string &s) {
	return AlgToRule(new StringRule(s));
}

Rule Hapy::anychar_r() {
	return AlgToRule(new AnyCharRule);
}

Rule Hapy::alpha_r() {
	return AlgToRule(new AlphaRule);
}

Rule Hapy::digit_r() {
	return AlgToRule(new DigitRule);
}

Rule Hapy::alnum_r() {
	return alpha_r() | digit_r();
}

Rule Hapy::space_r() {
	return AlgToRule(new SpaceRule);
}

Rule Hapy::eol_r() {
	// or could use "\r\n" | "\n";
	return string_r("\n");
}

Rule Hapy::quoted_r(char open, const Rule &middle, char close) {
	return quoted_r(char_r(open), middle, char_r(close));
}

Rule Hapy::quoted_r(const Rule &open, const Rule &middle, const Rule &close) {
	SeqRule *s = new SeqRule;
	s->add(open);
	s->add(*(middle-close));
	s->add(close);
	Rule r = AlgToRule(s);
	r.committed(true);
	//r.verbatim(true);
	return r;
}

Rule Hapy::empty_r() {
	return AlgToRule(new EmptyRule);
}

Rule Hapy::commit_r(const Rule &r) {
	Should(false);
	return r; // AlgToRule(new EmptyRule);
}

Rule Hapy::aggregate_r(const Rule &r) {
	Should(false);
	return r; // AlgToRule(new EmptyRule);
}

Rule Hapy::end_r() {
	return AlgToRule(new EndRule);
}

