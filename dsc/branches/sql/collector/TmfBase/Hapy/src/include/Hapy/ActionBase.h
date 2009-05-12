/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_ACTION_BASE__H
#define HAPY_ACTION_BASE__H


#include <Hapy/Result.h>

namespace Hapy {

class Buffer;
class Pree;

// actual parsing action interface (hidden behind Action wrapper):
// a rule can be augmented with an action to be performed when the
// rule matches; the action can confirm a match (default), cancel it,
// or return an error but cannot ask for more input (only rules can)
class ActionBase {
	public:
		typedef Result::StatusCode StatusCode;

		class Params {
			public:
				inline Params(Buffer &aBuf, Pree &aPree, StatusCode &aResult);

			public:
				Buffer &buf;
				Pree &pree;
				StatusCode &result;
		};

	public:
		virtual ~ActionBase() {}

		// "const" because action is an algorithm; it does not change
		void operator ()(Params *params) const { act(params); }

		// specifications override this method
		virtual void act(Params *params) const = 0;
};

inline
ActionBase::Params::Params(Buffer &aBuf, Pree &aPree, StatusCode &aResult):
	buf(aBuf), pree(aPree), result(aResult) {
}

} // namespace

#endif

