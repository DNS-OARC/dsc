 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef COAD_BASE_ANY_TO_STRING__H
#define COAD_BASE_ANY_TO_STRING__H

#include <strstream>
#include <string>

// expensive but convenient convertion of any printable object to string
template <class T>
inline
string AnyToString(const T &any) {
	ostrstream os;
	os << any;
	const string res = os.pcount() > 0 ?
		string(os.str(), os.pcount()) : string();
	os.freeze(false);
	return res;
}

// expensive but convenient convertion of any printable object to string
template <class T>
inline
string PrintToString(const T &any) {
	ostrstream os;
	any.print(os);
	const string res = os.pcount() > 0 ?
		string(os.str(), os.pcount()) : string();
	os.freeze(false);
	return res;
}

// no-op optimization
inline
string AnyToString(const string &s) {
	return s;
}

// cheaper convertion that requires knowing the maximum size
template <class T>
inline
string AnyToString(const T &any, string::size_type maxSize) {
	char buf[maxSize];
	ostrstream os(buf, sizeof(buf));
	os << any;
	const string res = os.pcount() > 0 ?
		string(os.str(), os.pcount()) : string();
	return res;
}

inline
string AnyToString(const long &any) {
	return AnyToString(any, 64);
}

// use this to change base; only 8, 10 and 16 bases are supported
extern string LongToString(long any, int base = 10); 

template <class Iterator>
inline
string AnyToString(Iterator first, Iterator last, const string &del) {
	string res;
	while (first != last) {
		if (res.size())
			res += del;
		res += AnyToString(*first);
		++first;
	}
	return res;
}

template <class Iterator>
inline
string AnyToString(Iterator first, Iterator last) {
	static const string del = ", ";
	return AnyToString(first, last, del);
}

#endif
