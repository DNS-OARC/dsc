/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_ALGORITHM__H
#define HAPY_ALGORITHM__H

#include <Hapy/Result.h>
#include <Hapy/HapyString.h>
#include <Hapy/IosFwd.h>
#include <Hapy/RulePtr.h>

namespace Hapy {

class Buffer;
class Pree;

// rule algorithm interface to build parsing tree while advancing input
// a parent of all rule implementations
class Algorithm {
	public:
		typedef Result::StatusCode StatusCode;
		typedef string::size_type size_type;

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

		virtual bool compile(const RulePtr &itrimmer) = 0;

		virtual ostream &print(ostream &os) const;

	protected:
		bool compileSubRule(RulePtr &r, const RulePtr &trimmer);
};

} // namespace


#endif
