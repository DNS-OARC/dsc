/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_ALGORITHMS__H
#define HAPY_ALGORITHMS__H

#include <Hapy/String.h>
#include <Hapy/Algorithm.h>
#include <Hapy/RulePtr.h>
#include <climits>
#include <vector>
#include <set>

namespace Hapy {

// r = empty sequence
class EmptyAlg: public Algorithm {
	public:
		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;

		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
};

// r = a >> b
class SeqAlg: public Algorithm {
	public:
		SeqAlg();

		void add(const RulePtr &rule);
		void addMany(const SeqAlg &rule);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool isA(const string &s) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual void collectRules(CRules &rules);

		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
		virtual void deepPrint(ostream &os, DeepPrinted &printed) const;

	protected:
		StatusCode advance(Buffer &buf, Pree &pree) const;
		StatusCode backtrack(Buffer &buf, Pree &pree) const;
		void killCurrent(Buffer &buf, Pree &pree) const;

	private:
		typedef std::vector<RulePtr> Store;
		Store theAlgs;
};

// r = a | b
class OrAlg: public Algorithm {
	public:
		void add(const RulePtr &rule);
		void addMany(const OrAlg &rule);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool isA(const string &s) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual void collectRules(CRules &rules);

		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
		virtual void deepPrint(ostream &os, DeepPrinted &printed) const;

	protected:
		StatusCode advance(Buffer &buf, Pree &pree) const;
		StatusCode nextMatchTail(Buffer &buf, Pree &pree) const;
		StatusCode backtrack(Buffer &buf, Pree &pree) const;

	private:
		typedef std::vector<RulePtr> Store;
		Store theAlgs;
};

// r = a - b
class DiffAlg: public Algorithm {
	public:
		DiffAlg(const RulePtr &aMatch, const RulePtr &anExcept);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual void collectRules(CRules &rules);

		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
		virtual void deepPrint(ostream &os, DeepPrinted &printed) const;

	protected:
		StatusCode checkAndAdvance(Buffer &buf, Pree &pree, StatusCode res) const;

	private:
		RulePtr theMatch;
		RulePtr theExcept;
};

// r = {min,max}a
class ReptionAlg: public Algorithm {
	public:
		ReptionAlg(const RulePtr &aAlg, size_type aMin = 0, size_type aMax = INT_MAX);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual void collectRules(CRules &rules);

		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
		virtual void deepPrint(ostream &os, DeepPrinted &printed) const;

	protected:
		StatusCode checkAndTry(Buffer &buf, Pree &pree, StatusCode res) const;
		StatusCode tryMore(Buffer &buf, Pree &pree) const;
		StatusCode backtrack(Buffer &buf, Pree &pree) const;

	private:
		RulePtr theAlg;
		size_type theMin; // minimum number of repetitions required
		size_type theMax; // maximum number of repetitions allowed
};

// r = a
class ProxyAlg: public Algorithm {
	public:
		ProxyAlg(const RulePtr &aAlg);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual void collectRules(CRules &rules);

		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
		virtual void deepPrint(ostream &os, DeepPrinted &printed) const;

	protected:
		StatusCode check(Buffer &buf, Pree &pree, StatusCode res) const;
		StatusCode backtrack(Buffer &buf, Pree &pree) const;

	private:
		RulePtr theAlg;
};

// r = "a"
class StringAlg: public Algorithm {
	public:
		StringAlg(const string &aToken);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;

	private:
		string theToken;
};

// r = single character from a known set
class CharSetAlg: public Algorithm {
	public:
		CharSetAlg(const string &aSetName);

		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;

	protected:
		virtual bool matchingChar(char c) const = 0;

	protected:
		string theSetName;
};

// r = any char
class AnyCharAlg: public CharSetAlg {
	public:
		AnyCharAlg();

		virtual bool calcPartialFirst(First &first, Pree &pree); // optimization

	protected:
		virtual bool matchingChar(char c) const;
};

// r = user-specified char set
class SomeCharAlg: public CharSetAlg {
	public:
		typedef std::set<char> Set;

	public:
		SomeCharAlg(const string &set);
		SomeCharAlg(const Set &set);

	protected:
		virtual bool matchingChar(char c) const;

	protected:
		Set theSet;
};

// r = first <= x <= last
class CharRangeAlg: public CharSetAlg {
	public:
		CharRangeAlg(char first, char last);

	protected:
		virtual bool matchingChar(char c) const;

	protected:
		char theFirst;
		char theLast;
};

// r = alpha
class AlphaAlg: public CharSetAlg {
	public:
		AlphaAlg();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = digit
class DigitAlg: public CharSetAlg {
	public:
		DigitAlg();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = space
class SpaceAlg: public CharSetAlg {
	public:
		SpaceAlg();

	protected:
		virtual bool matchingChar(char c) const;
};

// r = eof
class EndAlg: public Algorithm {
	public:
		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		virtual StatusCode resume(Buffer &buf, Pree &pree) const;
		virtual bool terminal(string *name = 0) const;

		virtual bool compile(const RuleCompFlags &flags);
		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		virtual bool calcPartialFirst(First &first, Pree &pree);
		virtual void calcFullFirst();

		virtual ostream &print(ostream &os) const;
};

} // namespace


#endif
