 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/Assert.h"
#include "Hapy/RuleAlg.h"

using namespace Hapy;

void Hapy::RuleAlg::cancel(Buffer &buf, PreeNode &pree) const {
	// XXX: optimize
	do {
		const Result::StatusCode sc = nextMatch(buf, pree);
		switch (sc.sc()) {
			case Result::scMiss:
			case Result::scError:
				return;
			case Result::scMore:
				Should(false); // XXX: we do not support this well
				return;
			case Result::scMatch:
				break; // the switch, not the loop
			default:
				Should(false);
				return;
		}
	} while (true);
}

bool Hapy::RuleAlg::isA(const string &) const {
	return false;
}

ostream &Hapy::RuleAlg::print(ostream &os) const {
	os << "rule algorithm";
	return os;
}
