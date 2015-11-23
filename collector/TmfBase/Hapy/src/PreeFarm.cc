/* Hapy is a public domain software. See Hapy README file for the details. */


#include <Hapy/Assert.h>
#include <Hapy/Pree.h>
#include <Hapy/PreeFarm.h>

Hapy::PreeFarm::Store Hapy::PreeFarm::TheStore = 0;
Hapy::PreeFarm::Count Hapy::PreeFarm::TheNewCount = 0;
Hapy::PreeFarm::Count Hapy::PreeFarm::TheGetCount = 0;
Hapy::PreeFarm::Count Hapy::PreeFarm::ThePutCount = 0;

Hapy::Pree *Hapy::PreeFarm::Get() {
	Should(!TheStore || TheStore->left == TheStore);
	++TheGetCount;
	if (Pree *p = TheStore) {
		Assert(TheStore->left == TheStore);
		if (TheStore->down) {
			p = TheStore->down;
			// pre-Put manipulations in Pree make Stored ups invalid; we fix
			p->up = TheStore;
			TheStore->rawPopChild(p);
			if (p->down)
				TheStore->pushChild(p->popSubTree());
		} else {
			TheStore = 0;
		}
		p->clear();
		return p;
	}
	++TheNewCount;
	return new Pree;
}

void Hapy::PreeFarm::Put(Pree *p) {
	Assert(p != TheStore);
	Should(!TheStore || TheStore->left == TheStore);
	if (Should(p)) {
		Should(p->left == p);
		p->up = 0;
		if (TheStore)
			TheStore->pushChild(p);
		else
			TheStore = p;
		++ThePutCount;
	}
}

void Hapy::PreeFarm::Clear() {
	while (TheStore)
		delete Get();
}
