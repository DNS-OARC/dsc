/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_ACTION__H
#define HAPY_ACTION__H


#include <Hapy/ActionBase.h>
#include <Hapy/PtrAction.h>

namespace Hapy {

// a wrapper around ActionBase (the true Action) to allow for implicit
// type conversion from function-looking actions to Action
class Action {
	public:
		typedef ActionBase::Params Params;

	public:
		Action(): base(0) {} // no action by default

		template <class Function>
		Action(Function f): base(new PtrAction<Function>(f)) {}

		operator void*() const { return base ? (void*)-1 : 0; }
		void clear() { base = 0; }

	public:
		const ActionBase *base; // action is an algorithm that does not change
};

} // namespace

#endif

