/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_FUN_ACTION__H
#define HAPY_FUN_ACTION__H


#include <Hapy/Action.h>

namespace Hapy {

// a "functional" implementation of the Action interface
// the function can be anything that accepts a pointer to Action::Params
template <class Function>
class PtrAction: public Action {
	public:
		PtrAction(Function aFunction): theFunction(aFunction) {}
		virtual void act(Params &params) const { theFunction(&params); }

	private:
		Function theFunction;
};

// use the following template to convert a function pointer to Action
// before supplying the pointer to Rule.action()
template <class Function>
inline
Action *ptr_action(Function aFunction) {
	return new PtrAction<Function>(aFunction);
}

// typical stand-alone Function profile, directly accepted by Rule
typedef void (*ActionPtr)(Action::Params *params);


// a wrapper for method-based actions;
// a method can be anything that accepts a pointer to Action::Params
template <class Class, class Method>
class MemAction: public Action {
	public:
		MemAction(Class *anObj, Method aMethod): theObj(anObj), theMethod(aMethod) {}
		virtual void act(Params &params) const { (theObj->*theMethod)(&params); }

	private:
		Class *theObj;
		Method theMethod;
};

// use the following template to convert a method pointer to Action
// before supplying the method to Rule.action()
template <class Class, class Method>
inline
Action *mem_action(Class *obj, Method method) {
	return new MemAction<Class,Method>(obj, method);
}

} // namespace

#endif

