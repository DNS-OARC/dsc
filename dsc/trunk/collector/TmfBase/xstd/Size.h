 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_SIZE__H
#define TMF_BASE__XSTD_SIZE__H

// reduces size convertion problems
// note: use BigSize for values of 2GB and higher
class Size {
	public:
		static Size KB(int n) { return Size(n * 1024); }
		static Size MB(int n) { return Size::KB(n * 1024); }
		static Size Byte(int n) { return Size(n); }

	public:
		Size(): theSize(-1) {}
		Size(int bytes): theSize(bytes) {}

		bool known() const { return theSize >= 0; }

		Size operator ++() { return ++theSize; }
		Size operator ++(int) { return theSize++; }
		Size operator += (Size sz) { return theSize += sz; }
		Size operator -= (Size sz) { return theSize -= sz; }
		Size operator *= (Size sz) { return theSize *= sz; }
		Size operator /= (Size sz) { return theSize /= sz; }
		Size operator %= (Size sz) { return theSize %= sz; }
		operator int() const { return theSize; }

		Size &operator =(int bytes) { theSize = bytes; return *this; }

		ostream &print(ostream &os) const;
		ostream &prettyPrint(ostream &os) const;

	private:
		int theSize;
};


/* operators */

inline
Size operator +(Size sz1, Size sz2) { return sz1 += sz2; }

inline
Size operator -(Size sz1, Size sz2) { return sz1 -= sz2; }

inline
double operator /(Size sz1, Size sz2) { return sz1 /= sz2; }

inline
Size operator *(Size sz, int f) { return sz *= f; }

inline
Size operator /(Size sz, int f) { return sz /= f; }

inline
ostream &operator <<(ostream &os, const Size &sz) { return sz.print(os); }

template <class Type>
inline
Size SizeOf(const Type &obj) {
	return (Size)sizeof(obj);
}

#endif
