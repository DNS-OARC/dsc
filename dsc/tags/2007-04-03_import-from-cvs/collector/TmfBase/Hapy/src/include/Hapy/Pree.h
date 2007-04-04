/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PREE__H
#define HAPY_PREE__H

#include <Hapy/config.h>
#include <Hapy/Area.h>
#include <Hapy/RuleId.h>
#include <Hapy/PreeKids.h>
#include <Hapy/HapyString.h>
#include <Hapy/IosFwd.h>

namespace Hapy {

// a node of a parse result tree
class Pree {
	public:
		typedef PreeKids Kids;
		typedef Kids::size_type size_type;
		typedef DerefIterator<Kids::iterator> iterator;
		typedef DerefIterator<Kids::const_iterator> const_iterator;

	public:
		Pree();
		Pree(const Pree &p);
		~Pree();

		void clear();

		// these are usually used by interpreters; they skip trimmings
		const RuleId &rid() const;
		size_type count() const;
		const_iterator begin() const;
		const_iterator end() const;
		const Pree &operator [](size_type idx) const;
		const Pree &find(const RuleId &id) const;
		const string &image() const;

		// these raw interfaces are usually used by parsers
		const RuleId &rawRid() const;
		void rawRid(const RuleId &aRid);
		size_type rawCount() const;
		size_type rawDeepCount() const;
		const Pree &backChild() const;
		const Pree &rawChild(size_type idx) const;
		Pree &backChild();
		Pree &newChild();
		void popChild();
		bool leftRecursion() const;
		bool emptyLoop() const;
		void commit();
		const string &rawImage() const;
		void rawImage(const string &anImage);

		std::ostream &print(std::ostream &os) const;
		std::ostream &print(std::ostream &os, const string &pfx) const;
		std::ostream &rawPrint(std::ostream &os, const string &pfx) const;

		// maybe slow; not optimized yet!
		Pree &operator =(const Pree &p);

	public:
		Area match;
		size_type idata;         // used by parsing rules to keep state
		Pree *parent;            // temporary value to kill infinite recursion
		Pree *nextFarmed;        // temporary value for farmed nodes

		bool implicit;
		bool leaf;

	protected:
		void clearKids();
		void kidlessAssign(const Pree &p);
		void copyKids(const Pree &source);
		void aboutToModify() {}
		const Pree &coreNode() const;
		const Pree *findSameUp(const Pree &n) const;
		bool sameState(const Pree &n) const;

	private:
		RuleId theRid;
		Kids theKids;
};

template <class Function>
void for_some(const Pree &n, const RuleId &rid, Function f) {
	for (Pree::const_iterator i = n.begin(); i < n.end(); ++i) {
		if (i->rid() == rid)
			(f)(*i);
		else
			for_some(*i, rid, f);
	}
}

template <class Function>
const Pree *find_if(const Pree &n, Function f) {
	for (Pree::const_iterator i = n.begin(); i < n.end(); ++i) {
		if ((f)(*i))
			return &(*i);
		if (const Pree *p = find_if(*i, f))
			return p;
	}
	return 0;
}

} // namespace

#endif

