 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_RULE__H
#define TMF_BASE__HAPY_RULE__H

#include <string>
#include "Hapy/Result.h"

namespace Hapy {

class Buffer;
class PreeNode;

class RuleAlg;
class Rule;

// rule information holder
class RuleBase {
	public:
		RuleBase();

	public:
		RuleAlg *alg;
		string name;
		int id;

		Rule *core;
		Rule *itrimmer;

		enum { cmDefault, cmDont, cmCommit } commitMode;
		enum { tmDefault, tmVerbatim, tmImplicit, tmExplicit } trimMode;
		bool isLeaf;

		bool compiling;
};

// a smart pointer to RuleBase
class RuleBasePtr {
	public:
		RuleBasePtr();
		RuleBasePtr(const RuleBasePtr &p);
		~RuleBasePtr();

		RuleBasePtr &operator =(const RuleBasePtr &p);

		RuleBase *operator ->() { return thePtr; }
		RuleBase &operator *() { return *thePtr; }
		const RuleBase *operator ->() const { return thePtr; }
		const RuleBase &operator *() const { return *thePtr; }

	private:
		RuleBase *thePtr;
};

// rule handler to support recursive, delayed, and proxied rule definitions
class Rule {
	public:
		typedef Result::StatusCode StatusCode;

	public:
		Rule();
		Rule(const Rule &r);
		Rule(const string &aName, int anId);
		Rule(const string &s); // converts to StringRule
		Rule(const char *s); // converts to StringRule
		~Rule();

		const string &name() const { return theBase->name; }
		int id() const { return theBase->id; }

		bool compile();

		void committed(bool be);

		bool terminal() const;
		bool trimming() const;
		void trim(const Rule &skipper);
		void implicitTrim(const Rule &itrimmer);
		void verbatim(bool be);
		void leaf(bool be);

		StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		StatusCode resume(Buffer &buf, PreeNode &pree) const;
		void cancel(Buffer &buf, PreeNode &pree) const;

		ostream &print(ostream &os) const;

		bool hasAlg() const { return theBase->alg != 0; }
		const RuleAlg &alg() const;
		void alg(RuleAlg *anAlg);

		// prevents base (name, id, trimming, etc.) re-initialization
		Rule &operator =(const Rule &r);
		Rule &operator =(const RuleAlg &a);

	protected:
		bool temporary() const;
		bool compileTrim();

	private:
		RuleBasePtr theBase;
};

} // namespace

#endif
