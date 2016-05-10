/* Hapy is a public domain software. See Hapy README file for the details. */


#include <Hapy/Pree.h>
#include <Hapy/PreeKids.h>
#include <Hapy/Assert.h>

Hapy::PreeFarm::Store Hapy::PreeFarm::TheStore;
Hapy::PreeFarm::Level Hapy::PreeFarm::TheInLevel = 0;
Hapy::PreeFarm::Level Hapy::PreeFarm::TheOutLevel = 0;

Hapy::Pree *Hapy::PreeFarm::Get() {
	++TheOutLevel;
	if (Pree *p = TheStore) {
		TheStore = p->nextFarmed;
		--TheInLevel;
		return p;
	}
	return new Pree;
}

void Hapy::PreeFarm::Put(Pree *p) {
	if (Should(p)) {
		p->clear(); // may call Put for kids
		p->nextFarmed = TheStore;
		TheStore = p;
		++TheInLevel;
		--TheOutLevel;
	}
}

void Hapy::PreeFarm::Clear() {
	while (TheStore) {
		delete Get();
		--TheOutLevel;
	}
}
