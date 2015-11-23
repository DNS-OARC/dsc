/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Result.h>

const Hapy::string &Hapy::Result::StatusCode::str() const {
	switch (sc()) {
		case scNone: {
			static const string scsNone = "none";
			return scsNone;
		}
		case scMore: {
			static const string scsMore = "more";
			return scsMore;
		}
		case scMatch: {
			static const string scsMatch = "match";
			return scsMatch;
		}
		case scMiss: {
			static const string scsMiss = "miss";
			return scsMiss;
		}
		case scError: {
			static const string scsError = "ERROR";
			return scsError;
		}
		default: {
			static const string scsUnknown = "UNNWN";
			return scsUnknown;
		}
	}
}

Hapy::string Hapy::Result::location() const {
	if (maxPos < input.size())
		return "near '" + input.substr(maxPos).substr(0, 40) + "'";
	else
		return string("near the end of input");
}
