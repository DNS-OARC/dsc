 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_RESULT__H
#define TMF_BASE__HAPY_RESULT__H

#include "Hapy/Pree.h"

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

			private:
				int theCode;
		};

		Result(): statusCode(scNone) {}

		bool known() const { return statusCode != scNone; }

	public:
		PreeNode pree;   // parsing tree
		StatusCode statusCode;  // final status code
};

} // namespace

#endif

