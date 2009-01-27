/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_SIZE_CALC__H
#define HAPY_SIZE_CALC__H

#include <Hapy/Top.h>
#include <Hapy/Pree.h>

namespace Hapy {

/* These classes are used by parsing rules to calculate minimum total size
   (length) of terminals they may consume */

// maintains size calculation state for one grammar graph pass
class SizeCalcPass {
	public:
		SizeCalcPass(void *id);

	public:
		void *id;
		int depth; // rules traversed from the starting point
		int minLoopDepth; // for loopless rule detection
};

// local size calculation state and result
class SizeCalcLocal {
	public:
		typedef string::size_type size_type;
		static const size_type nsize;

	public:
		SizeCalcLocal(); 

		bool withValue() const { return isSet; }
		int depth() const { return theDepth; }

		size_type value() const;
		void value(size_type aValue);

		bool doing(const SizeCalcPass &pass) const;
		//bool did(const SizeCalcPass &pass) const;
		void start(SizeCalcPass &pass);
		void stop(SizeCalcPass &pass); // updated pass.dept

	private:
		size_type theValue;  // computed value
		void *theDonePass;   // last completed pass
		int theDepth;        // the depth when this rule was first reached
		bool isSet;          // value is available
		bool isDoing;        // currently computing value
};

} // namespace

#endif

