 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef HAPY_PREE__H
#define HAPY_PREE__H

#include <string>
#include <vector>

namespace Hapy {

// a node of a parse result tree
class PreeNode {
	public:
		typedef vector<PreeNode> Children;
		typedef Children::size_type size_type;
		typedef Children::iterator iterator;
		typedef Children::const_iterator const_iterator;

	public:
		PreeNode();

		// these are usually used by interpreters; they skip trimmings
		int rid() const;
		size_type count() const;
		const_iterator begin() const;
		const_iterator end() const;
		const PreeNode &operator [](size_type idx) const;
		const string &image() const;
		void image(const string &anImage);

		// these raw interfaces are usually used by parsers
		int rawRid() const;
		void rawRid(int aRid);
		size_type rawCount() const;
		const PreeNode &backChild() const;
		const PreeNode &rawChild(size_type idx) const;
		PreeNode &backChild();
		PreeNode &newChild();
		void popChild();

		string::size_type rawImageSize() const;
		string rawImage() const;

		ostream &print(ostream &os) const;
		ostream &print(ostream &os, const string &pfx) const;
		ostream &rawPrint(ostream &os, const string &pfx) const;

	public:
		size_type idata;   // used by parsing rules to keep state

		bool trimming;
		bool leaf;

	protected:
		void aboutToModify();
		int trimFactor(int n) const;
		const PreeNode &coreNode() const;
		const string &trimmedImage() const;

	private:
		int theRid;                // rule ID
		Children theChildren;

		mutable string theImage;   // assigned or original image
		mutable enum { isKids, isOriginal, isCached } theImageState;
};

template <class Function>
void for_some(const PreeNode &n, int rid, Function f) {
	for (PreeNode::Children::const_iterator i = n.begin(); i < n.end(); ++i) {
		if (i->rid() == rid)
			(f)(*i);
		else
			for_some(*i, rid, f);
	}
}

} // namespace

#endif

