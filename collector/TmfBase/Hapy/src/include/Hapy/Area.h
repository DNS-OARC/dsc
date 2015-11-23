/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_AREA__H
#define HAPY_AREA__H

#include <Hapy/Top.h>
#include <Hapy/String.h>
#include <Hapy/IosFwd.h>

namespace Hapy {

// GCC STL implementation of string::substr() requires copying
// Area is a substring that delays copying until absolutely necessary
// Area points to an area of a buffer and keeps the latter so that the
// user does not have to; this behavior prevents buffer from being freed
class Area {
	public:
		typedef string::size_type size_type;

	public:
		Area();

		void clear();

		bool known() const { return theState != asNone; }
		size_type size() const { return theSize; }
		size_type start() const { return theStart; }

		const string &image() const; // expensive
		const char *imageData() const;
		size_type imageSize() const;
		bool operator ==(const Area &a) const;
		bool operator !=(const Area &a) const;

		void start(size_type pos) {
			theStart = pos;
		}

		void size(const string &aBuffer, size_type aSize) {
			theImage = aBuffer; // cheap
			theSize = aSize;
			theState = asSet;
		}

		void image(const string &anImage); // force a different image

	private:
		mutable string theImage; // buffer or frozen image, depending on state
		size_type theStart;
		size_type theSize;
		mutable enum { asNone, asSet, asFrozen } theState;
};

extern ostream &operator <<(ostream &os, const Area &a);

} // namespace

#endif

