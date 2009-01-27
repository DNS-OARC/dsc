/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PTR_ACTION__H
#define HAPY_PTR_ACTION__H

#include <Hapy/ActionBase.h>

namespace Hapy {

// a "functional" implementation of the Action interface
// the function can probably be anything with a copy constructor and
// a function call operator that accepts a pointer to Action::Params
template <class Function>
class PtrAction: public ActionBase {
	public:
		PtrAction(Function aFunction): theFunction(aFunction) {}
		virtual void act(Params *params) const { theFunction(params); }

	private:
		Function theFunction;
};

} // namespace

#endif

