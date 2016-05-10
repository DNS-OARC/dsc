/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_ALGORITHM__H
#define HAPY_ALGORITHM__H

#include <Hapy/Result.h>
#include <Hapy/String.h>
#include <Hapy/IosFwd.h>
#include <Hapy/SizeCalc.h>
#include <Hapy/DeepPrint.h>
#include <Hapy/RulePtr.h>
#include <list>

namespace Hapy {

class Buffer;
class Pree;
class RuleCompFlags;
class First;

// parsing algorithm interface
// builds parsing tree while advancing input
class Algorithm {
	public:
		typedef Result::StatusCode StatusCode;
		typedef string::size_type size_type;
		typedef std::list<RulePtr> CRules;

	public:
		virtual ~Algorithm() {}

		// find the first match
		virtual StatusCode firstMatch(Buffer &buf, Pree &pree) const = 0;

		// backtrack after success and find the next match
		virtual StatusCode nextMatch(Buffer &buf, Pree &pree) const = 0;

		// continue where scMore stopped us
		virtual StatusCode resume(Buffer &buf, Pree &pree) const = 0;

		// poor man's dynamic_cast; used to merge ">>" and "|" operands
		virtual bool isA(const string &typeName) const;

		// algorithm has no subrules
		virtual bool terminal(string *name = 0) const = 0;

		virtual bool compile(const RuleCompFlags &flags) = 0;
		virtual void collectRules(CRules &rules);

		virtual SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass) = 0;

		// FIRST calculation logic
		virtual void calcFullFirst() = 0;
		virtual bool calcPartialFirst(First &first, Pree &pree) = 0;

		virtual ostream &print(ostream &os) const;
		virtual void deepPrint(ostream &os, DeepPrinted &printed) const;

	protected:
		bool compileSubRule(RulePtr &r, const RuleCompFlags &flags);
};

} // namespace


#endif
