/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULE_COMP_FLAGS__H
#define HAPY_RULE_COMP_FLAGS__H

#include <Hapy/Top.h>
#include <Hapy/RulePtr.h>

namespace Hapy {

// Rule compilation flags to pass rule compilation info to subrules
class RuleCompFlags {
	public:
		RuleCompFlags(): trimmer(false), trimLeft(false), trimRight(false),
			ignoreCase(false), reachEnd(false) {}

		void enableTrim(RulePtr aTrimmer) {  trimmer = aTrimmer; 
				trimLeft = trimRight = (trimmer != 0); }

		void disableTrim() {  disableSideTrim(); trimmer = 0; }
		void disableSideTrim() {  trimLeft = trimRight = false; }

	public:
		RulePtr trimmer;
		bool trimLeft;
		bool trimRight;
		bool ignoreCase;
		bool reachEnd;   // buffer.sawEnd() must be true for rule to match
};

} // namespace

#endif

