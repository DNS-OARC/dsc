/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Assert.h>
#include <Hapy/RuleId.h>
#include <Hapy/NumericLimits.h>
#include <Hapy/IoStream.h>

Hapy::RuleId::Id Hapy::RuleId::ThePerm = 100;
Hapy::RuleId::Id Hapy::RuleId::TheTmp = -100;

Hapy::RuleId Hapy::RuleId::Next() {
	Assert(ThePerm < std::numeric_limits<Id>::max());
	return RuleId(++ThePerm);
}

Hapy::RuleId Hapy::RuleId::Temporary() {
	Assert(TheTmp > std::numeric_limits<Id>::min());
	return RuleId(--TheTmp);
}

void Hapy::RuleId::autoName(const string &hint) const {
	if (name().empty() && !hint.empty()) {
		string n = hint;
		if (n[0] != '_' || n.size() == 1)
			n = "_" + n;
		name(n.substr(0, 10));
	}
}

Hapy::ostream &Hapy::RuleId::print(ostream &os) const {
	if (theName.size())
		os << theName << '#';
	return os << 'r' << theId;
}

Hapy::ostream &Hapy::operator <<(ostream &os, const RuleId &id) {
	return id.print(os);
}
