/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/PreeFarm.h>
#include <Hapy/IoStream.h>
#include <Hapy/Pree.h>

#include <functional>
#include <algorithm>


Hapy::Pree::Pree():	up(0), down(0), left(0), right(0), kidCount(0),
	idata(0), minSize(0), implicit(false), leaf(false) {
	left = right = this;
}

// parent and nextFramed are not copied
Hapy::Pree::Pree(const Pree &p): up(0), down(0), left(0), right(0), kidCount(0),
	idata(p.idata), minSize(p.minSize), 
	implicit(p.implicit), leaf(p.leaf), theRid(p.theRid) {
	left = right = this;
	copyKids(p);
}

Hapy::Pree::~Pree() {
	clearKids();
}

void Hapy::Pree::clear() {
	match.clear();

	up = 0;
	clearKids();
	// these should already be correct; we are just trying to be robust here
	left = right = this;
	down = 0;
	kidCount = 0;

	idata = 0;
	minSize = 0;
	implicit = leaf = false;
	theRid.clear();
}

Hapy::Pree &Hapy::Pree::operator =(const Pree &p) {
	if (this != &p) {
		kidlessAssign(p);
		clearKids();
		copyKids(p);
	}
	return *this;
}

void Hapy::Pree::clearKids() {
	// we can only put well-formed trees and down may not be well-formed
	if (down) 
		PreeFarm::Put(popSubTree());
}

void Hapy::Pree::copyKids(const Pree &p) {
	Assert(!down);
	for (const_iterator i = p.rawBegin(); i != p.rawEnd(); ++i)
		newChild() = *i;
}

const Hapy::Pree &Hapy::Pree::coreNode() const {
	if (implicit) {
		Should(!leaf);
		const_iterator i = rawBegin();
		Assert(i != rawEnd());
		if (!i->implicit)
			return *i;
		++i;
		Assert(i != rawEnd());
		return i->coreNode();
	}
	return *this;
}

bool Hapy::Pree::deeplyImplicit() const {
	if (!implicit)
		return false;
	for (const_iterator i = rawBegin(); i != rawEnd(); ++i) {
		if (!i->deeplyImplicit())
			return false;
	}
	return true;
}

const Hapy::RuleId &Hapy::Pree::rid() const {
	return coreNode().theRid;
}

Hapy::Pree::size_type Hapy::Pree::count() const {
	const Pree &c = coreNode();
	return (leaf || c.leaf) ? 0 : c.rawCount();
}

Hapy::Pree::const_iterator Hapy::Pree::begin() const {
	const Pree &c = coreNode();
	return (leaf || c.leaf) ? c.rawEnd() : c.rawBegin();
}

Hapy::Pree::const_iterator Hapy::Pree::end() const {
	const Pree &c = coreNode();
	return c.rawEnd();
}

const Hapy::Pree &Hapy::Pree::operator [](size_type idx) const {
	const Pree &c = coreNode();
	Assert(!leaf && !c.leaf);
	return c.rawChild(idx);
}

const Hapy::Pree &Hapy::Pree::find(const RuleId &id) const {
	for (const_iterator i = begin(); i != end(); ++i) {
		if (i->rid() == id)
			return *i;
	}
	Assert(false);
	return *begin();
}

const Hapy::string &Hapy::Pree::image() const {
	return coreNode().rawImage();
}

const Hapy::string &Hapy::Pree::rawImage() const {
	return match.image(); // expensive
}


const Hapy::RuleId &Hapy::Pree::rawRid() const {
	return theRid;
}

void Hapy::Pree::rawRid(const RuleId &aRid) {
	theRid = aRid;
}

Hapy::Pree::size_type Hapy::Pree::rawCount() const {
	return kidCount;
}

Hapy::Pree::size_type Hapy::Pree::rawDeepCount() const {
	Hapy::Pree::size_type count = rawCount();
	for (const_iterator i = rawBegin(); i != rawEnd(); ++i)
		count += i->rawDeepCount();
	return count;	
}

Hapy::Pree::const_iterator Hapy::Pree::rawBegin() const {
	return const_iterator(down, 0);
}

Hapy::Pree::const_iterator Hapy::Pree::rawEnd() const {
	return const_iterator(down, kidCount);
}

const Hapy::Pree &Hapy::Pree::rawChild(size_type idx) const {
	// should we cache the last lookup, trading space for speed?
	Assert(down);
	Assert(0 <= idx && idx < kidCount);
	const Pree *res = down;
	if (idx <= kidCount/2) {
		for (; idx > 0; --idx)
			res = res->right;
	} else {
		for (idx = kidCount - idx; idx > 0; --idx)
			res = res->left;
	}
	Assert(res);
	return *res;
}

const Hapy::Pree &Hapy::Pree::top() const {
	return up ? up->top() : *this;
}

const Hapy::Pree &Hapy::Pree::backChild() const {
	Assert(down);
	return *down->left;
}

Hapy::Pree &Hapy::Pree::backChild() {
	Assert(down);
	return *down->left;
}

Hapy::Pree &Hapy::Pree::newChild() {
	Pree *p = PreeFarm::Get();
	pushChild(p);
	return *p;
}

void Hapy::Pree::popChild() {
	Pree *kid = down->left;
	rawPopChild(kid);
	PreeFarm::Put(kid);
}

void Hapy::Pree::rawPopChild(Pree *kid) {
	Assert(kid && kid != this && kid->up == this);
	Assert(down);
	Assert(kidCount > 0);
	if (--kidCount <= 0) {
		Should(down == kid);
		down = 0;
	} else {
		if (down == kid)
			down = kid->right;
		InsertAfter(kid->left, kid->right);
		kid->left = kid->right = kid;
	}
}

// extracts this->down and shapes it as a tree
Hapy::Pree *Hapy::Pree::popSubTree() {
	Assert(down);

	// reshape kids list into a tree if we have more than one kid
	Pree *top = down;
	if (top->left != top) { // not a well-formed tree
		Should(kidCount > 1);
		Pree *kids = down->left;
		HalfConnect(down->left, down->right); // closed kids loop
		top->left = top->right = top; // made an isolated top
		// note that kids "ups" are wrong now, but Farm does not care
		if (top->down) {
			top->kidCount += (kidCount - 1);
			InsertAfter(top->down->left, kids);
		} else {
			top->kidCount = (kidCount - 1);
			top->down = kids;
		}
	}

	down = 0;
	kidCount = 0;

	return top;
}

void Hapy::Pree::pushChild(Pree *p) {
	Assert(p->left == p); // or we would be pushing more than one kid
	if (down)
		InsertAfter(down->left, p);
	else
		down = p;
	p->up = this;
	++kidCount;
}

bool Hapy::Pree::emptyHorizontalLoop() const {
	if (rawCount() <= 1)
		return false;
	const Pree &last = backChild();
	// emulate reverse_iterator
	for (const Pree *i = down->left->left; i; i = i->left) {
		if (i->match.start() < last.match.start()) // progress made
			return false;
		if (i->sameState(last))
			return true;
		if (i == down)
			break;
	}
	return false;
}

bool Hapy::Pree::emptyVerticalLoop() const {
	for (const Pree *i = up; i; i = i->up) {
		if (i->minSize > 0) // accumulated debt (a form of progress)
			return false;
		if (i->match.start() < this->match.start()) // parsed something
			return false;
		if (i->rawRid() == rawRid()) // same rule, no progress
			return true;
	}
	return false;
}

Hapy::Pree::size_type Hapy::Pree::expectedMinSize() const {
	return minSize + (up ? up->expectedMinSize() : 0);
}

bool Hapy::Pree::sameState(const Pree &n) const {
	return n.rawRid() == rawRid() &&
		n.match.start() == match.start() && n.idata == idata;
}

bool Hapy::Pree::sameSegment(const Pree *them, bool &exhausted) const {
	exhausted = false;
	const Pree *us = up;
	while (us && them) {
		if (!us->sameState(*them))
			return false; // found different path component
		if (us->sameState(*this))
			return true; // both reached the end of the segment
		us = us->up;
		them = them->up;
	}
	exhausted = true;
	return false; // one segment is shorter or both too short
}

// optimization: remove empty trimming and unwanted kids
void Hapy::Pree::commit() {
	// best optimization possible: remove unwanted kids
	if (leaf) {
		clearKids();
		return;
	}

	// commit kids and remove empty implicit kids
	for (Pree *kid = down, *next = 0; kid; kid = next) {
		next = kid->right;
		if (next == down)
			next = 0;
		if (kid->match.size() == 0 && kid->deeplyImplicit()) {
			rawPopChild(kid);
			PreeFarm::Put(kid);
		} else {
			kid->commit(); // assume that commit does not change match.size
		}
	}

	// if an implicit node has only core kid left, replace it with the kid
	while (implicit && rawCount() == 1) {
		Pree *c = down;
		Assert(!(c->down == 0 && c->kidCount > 0));
		// be careful to move (not to copy) c and not destroy c's kids
		Should(match == c->match);
		kidlessAssign(*c);
		kidCount = c->kidCount;
		c->kidCount = 0;
		down = c->down; // raw
		c->down = 0; // raw
		// XXX: c kids' "up" points to c, is that bad? should caller s/this/c/?
		PreeFarm::Put(c);
	}
}

// assign everything but leave kids alone
void Hapy::Pree::kidlessAssign(const Pree &p) {
	// up, down, and kidCount
	match = p.match;
	idata = p.idata;
	minSize = p.minSize;
	implicit = p.implicit;
	leaf = p.leaf;
	theRid = p.theRid;
}

void Hapy::Pree::rawImage(const string &anImage) {
	aboutToModify();
	match.image(anImage);
}

Hapy::ostream &Hapy::Pree::print(ostream &os) const {
	return coreNode().print(os, "");
}

Hapy::ostream &Hapy::Pree::print(ostream &os, const string &pfx) const {
	size_type c = count();
	os << pfx << rid();
	if (c > 0)
		os << '(' << c << ')';
	os << ": " << coreNode().match << endl;
	if (c > 0) {
		const string p = pfx + "  ";
		for (const_iterator i = begin(); i != end(); ++i)
			i->print(os, p);
	}
	return os;
}

Hapy::ostream &Hapy::Pree::rawPrint(ostream &os, const string &pfx) const {
	os << pfx << rawRid() << " (" << rawCount() << "): '" << match << "'" <<
		" at " << match.start();
	if (minSize > 0)
		os << " msize: " << minSize;
	if (implicit)
		os << " implicit";
	if (leaf)
		os << " leaf";
	os << endl;
	if (rawCount()) {
		const string p = pfx + "  ";
		for (const_iterator i = rawBegin(); i != rawEnd(); ++i)
			i->rawPrint(os, p);
	}
	return os;
}
