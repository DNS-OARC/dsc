/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/Pree.h>
#include <Hapy/IoStream.h>

#include <functional>
#include <algorithm>


Hapy::Pree::Pree():	idata(0), parent(0), nextFarmed(0),
	implicit(false), leaf(false) {
	// clear();
}

// parent and nextFramed are not copied
Hapy::Pree::Pree(const Pree &p): idata(p.idata), parent(0), nextFarmed(0),
	implicit(p.implicit), leaf(p.leaf), theRid(p.theRid) {
	copyKids(p);
}

Hapy::Pree::~Pree() {
	clearKids();
}

void Hapy::Pree::clear() {
	match.clear();
	idata = 0;
	parent = nextFarmed = 0;
	implicit = leaf = false;
	theRid.clear();
	clearKids();
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
	for (Kids::iterator i = theKids.begin(); i != theKids.end(); ++i)
		PreeFarm::Put(*i);
	theKids.clear();
}

void Hapy::Pree::copyKids(const Pree &p) {
	for (Kids::const_iterator i = p.theKids.begin(); i != p.theKids.end(); ++i)
		newChild() = *(*i);
}

const Hapy::Pree &Hapy::Pree::coreNode() const {
	if (implicit) {
		Should(!leaf);
		Assert(theKids.size() > 0);
		if (!theKids[0]->implicit)
			return *theKids[0];
		Assert(theKids.size() > 1);
		if (!theKids[1]->implicit)
			return *theKids[1]; 
		Assert(false);
	}
	return *this;
}

const Hapy::RuleId &Hapy::Pree::rid() const {
	return coreNode().theRid;
}

Hapy::Pree::Kids::size_type Hapy::Pree::count() const {
	const Pree &c = coreNode();
	return (leaf || c.leaf) ? 0 : c.rawCount();
}

Hapy::Pree::const_iterator Hapy::Pree::begin() const {
	const Pree &c = coreNode();
	return (leaf || c.leaf) ? c.end() : const_iterator(c.theKids.begin());
}

Hapy::Pree::const_iterator Hapy::Pree::end() const {
	Kids::const_iterator e = coreNode().theKids.end();
	return const_iterator(e);
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

Hapy::Pree::Kids::size_type Hapy::Pree::rawCount() const {
	return theKids.size();
}

Hapy::Pree::Kids::size_type Hapy::Pree::rawDeepCount() const {
	Hapy::Pree::Kids::size_type count = rawCount();
	for (Kids::const_iterator i = theKids.begin(); i != theKids.end(); ++i)
		count += (*i)->rawDeepCount();
	return count;	
}

const Hapy::Pree &Hapy::Pree::rawChild(size_type idx) const {
	Assert(0 <= idx && idx < rawCount());
	return *theKids[idx];
}

const Hapy::Pree &Hapy::Pree::backChild() const {
	Assert(rawCount());
	return *theKids.back();
}

Hapy::Pree &Hapy::Pree::backChild() {
	Assert(rawCount());
	aboutToModify();
	return *theKids.back();
}

Hapy::Pree &Hapy::Pree::newChild() {
	aboutToModify();
	theKids.push_back(PreeFarm::Get());
	Pree &n = *theKids.back();
	n.parent = this;
	return n;
}

void Hapy::Pree::popChild() {
	Assert(rawCount());
	aboutToModify();
	PreeFarm::Put(theKids.back());
	theKids.pop_back();
}

bool Hapy::Pree::leftRecursion() const {
	return rawRid().known() && parent && parent->findSameUp(*this) != 0;
}

bool Hapy::Pree::emptyLoop() const {
	if (rawCount() <= 1)
		return false;
	const Pree &last = backChild();
	typedef Kids::const_reverse_iterator CRI;
	for (CRI i = ++theKids.rbegin(); i != theKids.rend(); ++i) {
		if ((*i)->match.start() < last.match.start()) // progress made
			return false;
		if ((*i)->sameState(last))
			return true;
	}
	return false;
}

const Hapy::Pree *Hapy::Pree::findSameUp(const Pree &n) const {
	Assert(parent != this);
	Should(&n != this);
	if (rawCount() == 1 && sameState(n))
		return this;
	else
	if (parent)
		return parent->findSameUp(n);
	else
		return 0;
}

bool Hapy::Pree::sameState(const Pree &n) const {
	return n.rawRid() == rawRid() &&
		n.match.start() == match.start() && n.idata == idata;
}

// optimization: remove empty trimming and unwanted kids
void Hapy::Pree::commit() {
	// best optimization possible: remove unwanted kids
	if (leaf) {
		clearKids();
		return;
	}

	// commit kids and remove empty implicit kids
	// cannot use an iterator because we want to erase stuff in the middle
	// unfortunately, vector storage is inefficient for this operation
	for (Kids::size_type i = 0; i < theKids.size();) {
		Pree *kid = theKids[i];
		if (kid->implicit && kid->match.size() == 0) {
			theKids.erase(theKids.begin() + i);
			PreeFarm::Put(kid);
		} else {
			kid->commit(); // assume that commit does not change match.size
			++i;
		}
	}

	// if an implicit node has only core kid left, replace it with the kid
	if (implicit && theKids.size() == 1) {
		Pree *c = theKids[0];
		// be careful to move (not to copy) c and not destroy c's kids
		Should(!c->implicit);
		Should(match == c->match);
		kidlessAssign(*c);
		theKids = c->theKids; // raw
		c->theKids.clear(); // raw
		// note: c kids' parent points to c but we do not care for now
		PreeFarm::Put(c);
	}
}

// assign everything but leave kids alone
void Hapy::Pree::kidlessAssign(const Pree &p) {
	// parent and nextFramed are not assigned
	match = p.match;
	idata = p.idata;
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
	Kids::size_type c = count();
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
	os << pfx << rawRid() << " (" << rawCount() << "): '" << match << "'";
	if (implicit)
		os << " implicit";
	if (leaf)
		os << " leaf";
	os << endl;
	if (rawCount()) {
		const string p = pfx + "  ";
		for (Kids::const_iterator i = theKids.begin(); i != theKids.end(); ++i)
			(*i)->rawPrint(os, p);
	}
	return os;
}
