 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_RULE_ALGS__H
#define TMF_BASE__HAPY_RULE_ALGS__H

#include <climits>
#include <string>
#include <vector>
#include "Hapy/RuleAlg.h"

namespace Hapy {

// r = empty sequence
class EmptyRule: public RuleAlg {
	public:
		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;
};

// r = a >> b
class SeqRule: public RuleAlg {
	public:
		SeqRule();

		void add(const Rule &rule);
		void addMany(const SeqRule &rule);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool isA(const string &s) const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	protected:
		StatusCode advance(Buffer &buf, PreeNode &pree) const;
		StatusCode nextMatchTail(Buffer &buf, PreeNode &pree) const;
		StatusCode backtrack(Buffer &buf, PreeNode &pree) const;

	private:
		typedef vector<Rule> Store;
		Store theRules;
};

// r = a | b
class OrRule: public RuleAlg {
	public:
		void add(const Rule &rule);
		void addMany(const OrRule &rule);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool isA(const string &s) const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	protected:
		StatusCode advance(Buffer &buf, PreeNode &pree) const;
		StatusCode nextMatchTail(Buffer &buf, PreeNode &pree) const;
		StatusCode backtrack(Buffer &buf, PreeNode &pree) const;

	private:
		typedef vector<Rule> Store;
		Store theRules;
};

// r = a - b
class DiffRule: public RuleAlg {
	public:
		DiffRule(const Rule &aMatch, const Rule &anExcept);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	protected:
		StatusCode checkAndAdvance(Buffer &buf, PreeNode &pree, StatusCode res) const;

	private:
		Rule theMatch;
		Rule theExcept;
};

// r = {min,max}a
class ReptionRule: public RuleAlg {
	public:
		ReptionRule(const Rule &aRule, size_type aMin = 0, size_type aMax = INT_MAX);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	protected:
		StatusCode checkAndTry(Buffer &buf, PreeNode &pree, StatusCode res) const;
		StatusCode tryMore(Buffer &buf, PreeNode &pree) const;
		StatusCode backtrack(Buffer &buf, PreeNode &pree) const;

	private:
		Rule theRule;
		size_type theMin; // minimum number of repetitions required
		size_type theMax; // maximum number of repetitions allowed
};

// r = a
class ProxyRule: public RuleAlg {
	public:
		ProxyRule(const Rule &aRule);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	protected:
		StatusCode check(Buffer &buf, PreeNode &pree, StatusCode res) const;
		StatusCode backtrack(Buffer &buf, PreeNode &pree) const;

	private:
		Rule theRule;
};

// r = "a"
class StringRule: public RuleAlg {
	public:
		StringRule(const string &aToken);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	private:
		string theToken;
};

// r = single character from a known set
class CharSetRule: public RuleAlg {
	public:
		CharSetRule(const string &aSetName);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;

	protected:
		virtual bool matchingChar(char c) const = 0;

	protected:
		string theSetName;
};

// r = any char
class AnyCharRule: public CharSetRule {
	public:
		AnyCharRule();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = alpha
class AlphaRule: public CharSetRule {
	public:
		AlphaRule();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = digit
class DigitRule: public CharSetRule {
	public:
		DigitRule();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = space
class SpaceRule: public CharSetRule {
	public:
		SpaceRule();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = eof
class EndRule: public RuleAlg {
	public:
		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual bool compile();
		virtual void spreadTrim(const Rule &itrimmer);

		virtual ostream &print(ostream &os) const;
};

#if 0
// aggregates images from all result nodes into one
class AggrRule: public RuleAlg {
	public:
		AggrRule(const Rule &aRule);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual ostream &print(ostream &os) const;

	protected:
		bool finish(Buffer &buf, PreeNode &pree, bool res) const;

	private:
		Rule theRule;
};

// stops backtracking
class CommitRule: public RuleAlg {
	public:
		CommitRule(const Rule &aRule);

		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const;
		virtual bool terminal() const;

		virtual ostream &print(ostream &os) const;

	private:
		Rule theRule;
};
#endif


} // namespace


#endif
