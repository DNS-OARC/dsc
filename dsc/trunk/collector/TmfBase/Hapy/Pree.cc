 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include "xstd/Assert.h"
#include "xstd/AnyToString.h"
#include "Hapy/Pree.h"

using namespace Hapy;


Hapy::PreeNode::PreeNode(): rid(0), idata(0), trimming(false),
	theImageState(isKids) {
}

int Hapy::PreeNode::trimFactor(int n) const {
	return trimming ? n : 0;
}

PreeNode::Children::size_type Hapy::PreeNode::count() const {
	return theChildren.size() - trimFactor(2);
}

PreeNode::const_iterator Hapy::PreeNode::begin() const {
	return theChildren.begin() + trimFactor(1);
}

PreeNode::const_iterator Hapy::PreeNode::end() const {
	return theChildren.end() - trimFactor(0);
}

const PreeNode &Hapy::PreeNode::operator [](int idx) const {
	return theChildren[idx + trimFactor(1)];
}

PreeNode::Children::size_type Hapy::PreeNode::rawCount() const {
	return theChildren.size();
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
	size_type s = image().size();
	if (trimming) {
		s += theChildren.begin()->rawImageSize();
		s += theChildren.rbegin()->rawImageSize();
	}
	return s;
}

const string &Hapy::PreeNode::image() const {
	if (theImageState == isKids) {
		theImageState = isCached;
		for (Children::const_iterator i = begin(); i != end(); ++i)
			theImage += i->image();
	}
	return theImage;
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
	return print(os, "");
}

ostream &Hapy::PreeNode::print(ostream &os, const string &pfx) const {
	os << pfx << rid << " (" << rawCount() << "): '" << image() << "'";
	if (trimming)
		os << " trimming";
	os << endl;
	if (rawCount()) {
		const string p = pfx + "  ";
		for (Children::const_iterator i = theChildren.begin(); i != theChildren.end(); ++i)
			i->print(os, p);
	}
	return os;
}

