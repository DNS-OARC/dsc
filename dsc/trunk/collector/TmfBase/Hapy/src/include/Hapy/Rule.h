/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULE__H
#define HAPY_RULE__H

#include <Hapy/config.h>
#include <Hapy/Result.h>
#include <Hapy/String.h>
#include <Hapy/IosFwd.h>
#include <Hapy/FunAction.h>
#include <Hapy/RulePtr.h>

namespace Hapy {

class Buffer;
class Pree;

class Algorithm;

// user-level grammar rule interface
// supports user-level semantics of rule creation, assignment, and manipulation
class Rule {
	public:
		static void Debug(bool doIt);

	public:
		Rule();
		Rule(const RulePtr &aBase);
		Rule(const Rule &r);
		Rule(const string &aName, RuleId *id); // user-level; returns id
		Rule(const string &s); // converts to string_r
		Rule(const char *s); // converts to string_r
		Rule(char c); // converts to char_r
		~Rule();

		const RuleId &id() const;

		void committed(bool be);
		void trim(const Rule &skipper);
		void verbatim(bool be);
		void leaf(bool be);
		void action(const Action *a);
		void ptr_action(ActionPtr p);

		ostream &print(ostream &os) const;

		// prevents base (id, trimming, etc.) re-initialization
		Rule &operator =(const Rule &r);
		Rule &operator =(const Algorithm &a);

		// access to the implementation-level interface
		RulePtr &base() { return theBase; }
		const RulePtr &base() const { return theBase; }

	private:
		Rule(int); // to prevent "Rule r = 5;" from using Rule(char)

	protected:
		RulePtr theBase;
};

} // namespace

#endif
