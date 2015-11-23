/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PREE__H
#define HAPY_PREE__H

#include <Hapy/Top.h>
#include <Hapy/Area.h>
#include <Hapy/RuleId.h>
#include <Hapy/PreeKids.h>
#include <Hapy/String.h>
#include <Hapy/IosFwd.h>

namespace Hapy {

// a node of a parse result tree
class Pree {
	public:
		typedef unsigned int size_type;
		typedef PreeKidsIterator<Pree> iterator;
		typedef PreeKidsIterator<const Pree> const_iterator;

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
		const Pree &top() const;
		const Pree &backChild() const;
		const Pree &rawChild(size_type idx) const;
		Pree &backChild();
		Pree &newChild();
		void popChild();  // extracts and destroys the last child
		void rawPopChild(Pree *kid); // extracts specified child
		void pushChild(Pree *kid); // adds the last child
		Pree *popSubTree(); // extracts this->down and shapes it as a tree

		bool emptyHorizontalLoop() const;
		bool emptyVerticalLoop() const;
		size_type expectedMinSize() const;
		void commit();
		const string &rawImage() const;
		void rawImage(const string &anImage);
		const_iterator rawBegin() const;
		const_iterator rawEnd() const;

		std::ostream &print(std::ostream &os) const;
		std::ostream &print(std::ostream &os, const string &pfx) const;
		std::ostream &rawPrint(std::ostream &os, const string &pfx) const;

		// maybe slow; not optimized yet!
		Pree &operator =(const Pree &p);

	protected:
		inline static void InsertAfter(Pree *p1, Pree *p2);
		inline static void HalfConnect(Pree *p1, Pree *p2);

	public:
		Area match;

		Pree *up;        // parent or null
		Pree *down;      // kids or null
		Pree *left;      // previous sibling or self
		Pree *right;     // next sibling or self
		size_type kidCount; // number of children

		size_type idata;   // used by parsing rules to keep state
		size_type minSize; // used by parsing rules to keep state

		bool implicit;
		bool leaf;

	protected:
		bool deeplyImplicit() const;
		void clearKids();
		void kidlessAssign(const Pree &p);
		void copyKids(const Pree &source);
		void aboutToModify() {}
		const Pree &coreNode() const;

		bool sameState(const Pree &n) const;
		bool sameSegment(const Pree *them, bool &exhausted) const;

	private:
		RuleId theRid;  
};

template <class Function>
inline
void for_some(const Pree &n, const RuleId &rid, Function f) {
	for (Pree::const_iterator i = n.begin(); i < n.end(); ++i) {
		if (i->rid() == rid)
			(f)(*i);
		else
			for_some(*i, rid, f);
	}
}

inline
const Pree *find_first(const Pree &n, const RuleId &rid) {
	for (Pree::const_iterator i = n.begin(); i < n.end(); ++i) {
		if (i->rid() == rid)
			return &(*i);
		if (const Pree *p = find_first(*i, rid))
			return p;
	}
	return 0;
}

template <class Function>
inline
const Pree *find_if(const Pree &n, Function f) {
	for (Pree::const_iterator i = n.begin(); i < n.end(); ++i) {
		if ((f)(*i))
			return &(*i);
		if (const Pree *p = find_if(*i, f))
			return p;
	}
	return 0;
}

inline
void Pree::HalfConnect(Pree *p1, Pree *p2) {
	p1->right = p2; // ignores p1->left
	p2->left = p1;  // ignores p2->right
}

inline
void Pree::InsertAfter(Pree *p1, Pree *p2) {
	Pree *p4 = p1->right;
	Pree *p3 = p2->left;
	HalfConnect(p1, p2);
	HalfConnect(p3, p4);
}


} // namespace

#endif

