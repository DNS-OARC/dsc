 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_RULE_ALG__H
#define TMF_BASE__HAPY_RULE_ALG__H

#include <string>
#include "Hapy/Result.h"

namespace Hapy {

class Buffer;
class PreeNode;
class Rule;

// rule algorithm interface to build parsing tree while advancing input
// a parent of all rule implementations
class RuleAlg {
	public:
		typedef Result::StatusCode StatusCode;
		typedef string::size_type size_type;

	public:
		virtual ~RuleAlg() {}

		// find the first match
		virtual StatusCode firstMatch(Buffer &buf, PreeNode &pree) const = 0;

		// backtrack after success and find the next match
		virtual StatusCode nextMatch(Buffer &buf, PreeNode &pree) const = 0;

		// continue where scMore stopped us
		virtual StatusCode resume(Buffer &buf, PreeNode &pree) const = 0;

		// cancel all effects (i.e., backtrack); XXX: can be slow for now
		void cancel(Buffer &buf, PreeNode &pree) const;

		// poor man's dynamic_cast; used to merge ">>" and "|" operands
		virtual bool isA(const string &typeName) const;

		// algorithm has no subrules
		virtual bool terminal() const = 0;

		virtual bool compile() = 0;
		virtual void spreadTrim(const Rule &itrimmer) = 0;

		virtual ostream &print(ostream &os) const;
};

} // namespace


#endif
