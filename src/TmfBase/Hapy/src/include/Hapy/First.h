/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_FIRST__H
#define HAPY_FIRST__H

#include <Hapy/Top.h>
#include <set>

namespace Hapy {

// FIRST set
// currently we only store the first character of FIRST terminals
// future optimizations may include storing complete FIRST terminals
class First {
	public:
		First();

		bool empty() const { return theSet.empty() && !hasEmpty(); }

		void clear();

		bool has(char c) const { return theSet.find(c) != theSet.end(); }
		bool hasEmpty() const { return hasEmptySeq; }

		First &operator +=(const First &f); // union
		First &operator -=(const First &f); // difference

		void include(char c); // just one
		void includeAny(); // but empty
		void includeRange(int first, int last);
		void includeEmptySequence(bool doIt);

	private:
		typedef std::set<char> Set; // using bitset would probably be faster
		Set theSet;
		bool hasEmptySeq; // includes empty sequence
};

} // namespace

#endif

