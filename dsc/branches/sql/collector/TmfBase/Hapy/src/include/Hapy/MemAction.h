/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_MEM_ACTION__H
#define HAPY_MEM_ACTION__H


#include <Hapy/ActionBase.h>

namespace Hapy {

// a wrapper for method-based actions;
// any method that accepts a pointer to Action::Params should work
template <class Class, class Method>
class MemAction: public ActionBase {
	public:
		MemAction(Class *anObj, Method aMethod):
			theObj(anObj), theMethod(aMethod) {
		}

		virtual void act(Params *params) const {
			(theObj->*theMethod)(params); 
		}

	private:
		Class *theObj;
		Method theMethod;
};

// use the following template to convert a method pointer to Action[Base]
// before supplying the method to Rule.action()
template <class Class, class Method>
inline
MemAction<Class,Method> mem_action(Class *obj, Method method) {
	return MemAction<Class,Method>(obj, method);
}

} // namespace

#endif

