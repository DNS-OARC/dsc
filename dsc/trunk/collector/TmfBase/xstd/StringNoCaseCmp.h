 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_STRING_NO_CASE_CMP__H
#define TMF_BASE__XSTD_STRING_NO_CASE_CMP__H

#include <ctype.h>
#include <string>


// case-insensitive comparison operators for string maps and such

// less_than
class NoCaseLessThan {
	public:
		inline bool operator()(const string &s1, const string &s2) const;
};

inline
bool NoCaseLessThan::operator()(const string &s1, const string &s2) const {
	string::const_iterator i1 = s1.begin();
	string::const_iterator i2 = s2.begin();
	
	while (i1 != s1.end() && i2 != s2.end() && toupper(*i1) == toupper(*i2)) {
		++i1;
		++i2;
	}

	if (i1 != s1.end() && i2 != s2.end())
		return toupper(*i1) < toupper(*i2);

	return i1 == s1.end() && i2 != s2.end();
}

// equal_to
class NoCaseEqualTo {
	public:
		NoCaseEqualTo() {}
		NoCaseEqualTo(const string &aStr): theStr(aStr) {}
	
		inline bool operator()(const string &s1, const string &s2) const;
		inline bool operator()(const string &s) const;

	private:
		const string theStr;
};

inline
bool NoCaseEqualTo::operator()(const string &s1, const string &s2) const {
	string::const_iterator i1 = s1.begin();
	string::const_iterator i2 = s2.begin();
	
	while (i1 != s1.end() && i2 != s2.end() && toupper(*i1) == toupper(*i2)) {
		++i1;
		++i2;
	}

	return i1 == s1.end() && i2 == s2.end();
}

inline
bool NoCaseEqualTo::operator()(const string &s) const {
	return (*this)(theStr, s);
}

#endif
