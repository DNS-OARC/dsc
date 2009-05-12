/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULE_PTR__H
#define HAPY_RULE_PTR__H

#include <Hapy/Top.h>

namespace Hapy {

class RuleBase;

// a smart pointer to RuleBase, to eventually support base refcounting
// all internal classes point to rules using this class
#if FUTURE_CODE
class RulePtr {
	public:
		RulePtr(RuleBase *b = 0);
		RulePtr(const RulePtr &p);
		~RulePtr();

		operator void*() const { return thePtr; }
		bool operator !() const { return !thePtr; }

		bool operator ==(const RulePtr &p) { return thePtr == p.thePtr; }
		bool operator !=(const RulePtr &p) { return !(*this == p); }

		RulePtr &operator =(const RulePtr &p);

		RuleBase *operator ->() { return thePtr; }
		RuleBase &operator *() { return *thePtr; }
		const RuleBase *operator ->() const { return thePtr; }
		const RuleBase &operator *() const { return *thePtr; }

	private:
		RuleBase *thePtr;
};
#else
	typedef RuleBase *RulePtr;
#endif

} // namespace

#endif
