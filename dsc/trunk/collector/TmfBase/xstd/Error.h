 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_ERROR__H
#define TMF_BASE__XSTD_ERROR__H

#include <errno.h>

class ostream;

// errno wrapper
class Error {
	public:
		static const Error None() { return Error(); }
		static const Error Last();
		static const Error Last(const Error &err);
		static const Error LastExcept(const Error &err);

	public:
		Error(int aNo = 0): theNo(aNo) {}

		operator void*() const { return no() ? (void*)-1 : (void*)0; }
		bool operator !() const { return !no(); }
		bool operator ==(const Error &e) const { return theNo == e.theNo; }
		bool operator !=(const Error &e) const { return !(e == *this); }

		int no() const { return theNo; }

		// use immediately; may return a pointer to shared memory
		const char *str() const;

		ostream &print(ostream &os) const { return print(os, str()); }

	protected:
		ostream &print(ostream &os, const char *str) const;

	protected:
		int theNo;		
};

inline
ostream &operator <<(ostream &os, const Error &err) {
	return err.print(os);
}

#endif
