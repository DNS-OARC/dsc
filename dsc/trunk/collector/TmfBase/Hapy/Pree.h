 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef HAPY_PREE__H
#define HAPY_PREE__H

#include <string>
#include <vector>

namespace Hapy {

// a node of a parse result tree
class PreeNode {
	public:
		typedef string::size_type size_type;
		typedef vector<PreeNode> Children;
		typedef Children::iterator iterator;
		typedef Children::const_iterator const_iterator;

	public:
		PreeNode();


		// these are usually used by interpreters; they skip trimmings
		Children::size_type count() const;
		const_iterator begin() const;
		const_iterator end() const;
		const PreeNode &operator [](int idx) const;

		// these raw interfaces are usually used by parsers
		Children::size_type rawCount() const;
		const PreeNode &backChild() const;
		PreeNode &backChild();
		PreeNode &newChild();
		void popChild();

		size_type rawImageSize() const;
		string rawImage() const;

		const string &image() const;
		void image(const string &anImage);

		ostream &print(ostream &os) const;
		ostream &print(ostream &os, const string &pfx) const;

	public:
		int rid;                   // rule ID
		string::size_type idata;   // used by parsing rules to keep state
		bool trimming;

	protected:
		void aboutToModify();
		int trimFactor(int n) const;

	private:
		mutable string theImage;   // assigned or original image
		mutable enum { isKids, isOriginal, isCached } theImageState;

		Children theChildren;
};

} // namespace

#endif

