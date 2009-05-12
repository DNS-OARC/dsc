/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULE_ID__H
#define HAPY_RULE_ID__H

#include <Hapy/IosFwd.h>
#include <Hapy/String.h>

namespace Hapy {

// opaque unique rule identifier with an optional human-friendly name
// the name part, if set, is only used for reporting
class RuleId {
	public:
		static RuleId Next();
		static RuleId Temporary();

	public:
		RuleId(): theId(0) {}

		const string &name() const { return theName; }
		void name(const string &aName) const { theName = aName; }
		void autoName(const string &hint) const;

		bool known() const { return theId != 0; }
		bool temporary() const { return theId < 0; }

		bool operator ==(const RuleId &id) const { return theId == id.theId; }
		bool operator !=(const RuleId &id) const { return !(*this == id); }
		
		ostream &print(ostream &os) const;

		void clear() { theId = 0; theName.erase(); }

	private:
		typedef signed long int Id;

		RuleId(Id anId): theId(anId) {}

	private:
		static Id ThePerm;
		static Id TheTmp;

		mutable string theName;
		Id theId;
};

extern ostream &operator <<(ostream &os, const RuleId &a);

} // namespace

#endif
