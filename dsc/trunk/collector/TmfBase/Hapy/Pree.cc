 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include "xstd/Assert.h"
#include "xstd/AnyToString.h"
#include "Hapy/Pree.h"

using namespace Hapy;


Hapy::PreeNode::PreeNode(): idata(0), trimming(false), leaf(false),
	theRid(0), theImageState(isKids) {
}

int Hapy::PreeNode::trimFactor(int n) const {
	return trimming ? n : 0;
}

const PreeNode &Hapy::PreeNode::coreNode() const {
	if (trimming && Should(rawCount() == 3)) {
		Assert(!theChildren[1].trimming); // XXX:remove paranoid
		return theChildren[1];
	}
	return *this;
}

int Hapy::PreeNode::rid() const {
	return coreNode().theRid;
}
PreeNode::Children::size_type Hapy::PreeNode::count() const {
	const PreeNode &c = coreNode();
	return (leaf || c.leaf) ? 0 : c.rawCount();
}

PreeNode::const_iterator Hapy::PreeNode::begin() const {
	const PreeNode &c = coreNode();
	return (leaf || c.leaf) ? c.end() : c.theChildren.begin();
}

PreeNode::const_iterator Hapy::PreeNode::end() const {
	return coreNode().theChildren.end();
}

const PreeNode &Hapy::PreeNode::operator [](int idx) const {
	const PreeNode &c = coreNode();
	Assert(!leaf && !c.leaf);
	return c.rawChild(idx);
}

const string &Hapy::PreeNode::image() const {
	return trimmedImage();
}

const string &Hapy::PreeNode::trimmedImage() const {
	if (theImageState == isKids) {
		theImageState = isCached;
		theImage = string();
		Children::const_iterator b = theChildren.begin();
		Children::const_iterator e = theChildren.end();
		if (trimming) {
			++b;
			--e;
		}
		for (Children::const_iterator i = b; i != e; ++i)
			theImage += i->rawImage();
	}
	return theImage;
}


int Hapy::PreeNode::rawRid() const {
	return theRid;
}

void Hapy::PreeNode::rawRid(int aRid) {
	theRid = aRid;
}

PreeNode::Children::size_type Hapy::PreeNode::rawCount() const {
	return theChildren.size();
}

const PreeNode &Hapy::PreeNode::rawChild(int idx) const {
	Assert(0 <= idx && idx < rawCount());
	return theChildren[idx];
}

const PreeNode &Hapy::PreeNode::backChild() const {
	Assert(rawCount());
	return theChildren.back();
}

PreeNode &Hapy::PreeNode::backChild() {
	Assert(rawCount());
	aboutToModify();
	return theChildren.back();
}

PreeNode &Hapy::PreeNode::newChild() {
	theChildren.resize(rawCount() + 1);
	aboutToModify();
	return theChildren.back();
}

void Hapy::PreeNode::popChild() {
	Assert(rawCount());
	aboutToModify();
	theChildren.pop_back();
}

PreeNode::size_type Hapy::PreeNode::rawImageSize() const {
	// XXX: optimize using positions
	size_type s = trimmedImage().size();
	if (trimming) {
		s += theChildren.begin()->rawImageSize();
		s += theChildren.rbegin()->rawImageSize();
	}
	return s;
}

string Hapy::PreeNode::rawImage() const {
	string s = trimmedImage();
	return trimming ?
		theChildren.begin()->rawImage() + s + theChildren.rbegin()->rawImage():
		s;
}

void Hapy::PreeNode::image(const string &anImage) {
	aboutToModify();
	theImage = anImage;
	theImageState = isOriginal;
}

void Hapy::PreeNode::aboutToModify() {
	if (theImageState == isCached) {
		theImageState = isKids;
		theImage = string();
	}
}

ostream &Hapy::PreeNode::print(ostream &os) const {
	return coreNode().print(os, "");
}

ostream &Hapy::PreeNode::print(ostream &os, const string &pfx) const {
	os << pfx << rid() << " (" << count() << "): '" << image() << "'";
	if (coreNode().trimming)
		os << " TRIMMING";
	if (leaf || coreNode().leaf)
		os << " leaf";
	if (leaf)
		os << "-this";
	os << endl;
	if (count()) {
		const string p = pfx + "  ";
		for (Children::const_iterator i = begin(); i != end(); ++i)
			i->print(os, p);
	}
	return os;
}

ostream &Hapy::PreeNode::rawPrint(ostream &os, const string &pfx) const {
	os << pfx << rawRid() << " (" << rawCount() << "): '" << rawImage() << "'";
	if (trimming)
		os << " trimming";
	if (leaf)
		os << " leaf";
	os << endl;
	if (rawCount()) {
		const string p = pfx + "  ";
		for (Children::const_iterator i = theChildren.begin(); i != theChildren.end(); ++i)
			i->rawPrint(os, p);
	}
	return os;
}
