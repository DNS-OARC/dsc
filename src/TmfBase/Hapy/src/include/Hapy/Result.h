/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RESULT__H
#define HAPY_RESULT__H

#include <Hapy/Top.h>
#include <Hapy/Pree.h>

namespace Hapy {

// parsing result
class Result {
	public:
		enum { scNone, scMore, scMatch, scMiss, scError };
		class StatusCode {
			public:
				StatusCode(int aCode = scNone): theCode(aCode) {}

				bool operator ==(const StatusCode &sc) const { return sc.theCode == theCode; }
				bool operator !=(const StatusCode &sc) const { return !(*this == sc); }

				int sc() const { return theCode; }
				const string &str() const;

			private:
				int theCode;
		};

		Result(): statusCode(scNone), maxPos(0) {}

		bool known() const { return statusCode != scNone; }

		string location() const;

	public:
		Pree pree;   // parsing tree
		StatusCode statusCode;  // final status code
		string::size_type maxPos; // maximum input position reached
		string input;  // a copy of the input string
};

} // namespace

#endif

