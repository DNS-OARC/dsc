 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_RULE__H
#define TMF_BASE__HAPY_RULE__H

#include <string>
#include "Hapy/Result.h"

namespace Hapy {

class Buffer;
class PreeNode;

class RuleAlg;

// a smart algorithm pointer to support recursive and yet-undefined rules
class RuleAlgPtr {
	public:
		RuleAlgPtr();
		RuleAlgPtr(const RuleAlgPtr &p);
		~RuleAlgPtr();

		void reset();

		RuleAlgPtr &operator =(const RuleAlgPtr &p);

		bool known() const;
		RuleAlg &value();
		const RuleAlg &value() const;
		void value(RuleAlg *anAlg);
		void repoint(RuleAlg *anAlg);

	private:
		struct Link {
			mutable RuleAlg *alg;
			Link *next;

			Link();
			const Link *find() const;
		};
		Link *theLink;
};

// rule holder to support recursive rule definitions
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

		bool anonymous() const;
		const string &name() const { return theName; }
		int id() const { return theId; }

		void committed(bool be);
		bool trimming() const;
		void trim(const Rule &skipper);

		StatusCode firstMatch(Buffer &buf, PreeNode &pree) const;
		StatusCode nextMatch(Buffer &buf, PreeNode &pree) const;
		StatusCode resume(Buffer &buf, PreeNode &pree) const;
		void cancel(Buffer &buf, PreeNode &pree) const;

		ostream &print(ostream &os) const;

		bool hasAlg() const { return theAlg.known(); }
		const RuleAlg &alg() const;
		void alg(RuleAlg *anAlg);

		// prevents name and id re-initialization
		Rule &operator =(const Rule &r);

	protected:
		void init(const Rule &r);

	protected:
		RuleAlgPtr theAlg;
		string theName;
		int theId;

		Rule *theCore;

		enum { cmDefault, cmDont, cmCommit } theCommitMode;
};

} // namespace

#endif
