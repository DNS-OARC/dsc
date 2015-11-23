		/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/PrettyPrint.h>
#include <Hapy/IoStream.h>
#include <iomanip>
#include <ctype.h>

void Hapy::PrettyPrint(ostream &os, const char *buffer, string::size_type bufSize, string::size_type maxShow) {
	const string::size_type end = maxShow < bufSize ? maxShow : bufSize;
	for (string::size_type i = 0; i < end; ++i) {
		const char c = buffer[i];
		if (isprint(c) && c != '\\') {
			os << c;
		} else
		switch (c) {
			case '\n':
				os << "\\n";
				break;
			case '\r':
				os << "\\r";
				break;
			case '\t':
				os << "\\t";
				break;
			case '\\':
				os << "\\\\";
				break;
			default:
				// XXX: restore original os settings or do not change them
				os << '\\' << std::setfill('0') << std::hex << std::setw(2) <<
					((unsigned)c) << std::dec;
		}
	}

	if (maxShow < bufSize) {
		static const string more = "...";
		os << more;
	}
}

