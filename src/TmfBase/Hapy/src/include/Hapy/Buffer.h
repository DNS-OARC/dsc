/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_BUFFER__H
#define HAPY_BUFFER__H

#include <Hapy/Rules.h>
#include <Hapy/String.h>
#include <Hapy/IosFwd.h>
#include <vector>


namespace Hapy {

class Buffer {
	public:
		typedef string::size_type size_type;

		Buffer();
		Buffer(const string &aData);

		void reset();
		void moveOn();

		bool empty() const { return pos() >= theData.size(); }
		size_type contentSize() const { return theData.size() - pos(); }
		string content(const size_type off = 0) const;
		char peek(const size_type off = 0) const { return theData[pos() + off]; }
		bool startsWith(const string &s, bool &needMore) const;
		bool sawEnd() const { return didSeeEnd; }

		void append(const string &aData);
		void sawEnd(bool did) { didSeeEnd = did; } // may still have data

		void advance(size_type delta);
		void backtrack(size_type delta);
		void backtrackTo(size_type pos);

		string allContent() const { return theData; }
		string parsedContent() const { return theData.substr(0, pos()); }
		size_type parsedSize() const { return pos(); }
		string maxProgress() const { return theData.substr(0, theMaxProgress); }

		void print(ostream &os, size_type maxSize) const;

	protected:
		size_type pos() const;

	protected:
		string theData;
		size_type thePos;
		size_type theMaxProgress;
		bool didSeeEnd;
};

} // namespace


#endif
