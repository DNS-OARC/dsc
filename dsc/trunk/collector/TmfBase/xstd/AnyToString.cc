 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include "xstd/iomanip.h"
#include "xstd/AnyToString.h"


string LongToString(long any, int base) {
	char buf[64];
	ostrstream os(buf, sizeof(buf));
	os << setbase(base) << any;
	const string res = os.pcount() > 0 ?
		string(os.str(), os.pcount()) : string();
	return res;
}
