 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_BUFFER__H
#define TMF_BASE__HAPY_BUFFER__H

#include <string>
#include <vector>
#include "Hapy/Rules.h"


namespace Hapy {

class Buffer {
	public:
		typedef string::size_type size_type;

		Buffer();
		Buffer(const string &aData);

		void skipper(Rule &aSkipper);

		bool empty() const { return pos() >= theData.size(); }
		size_type contentSize() const { return theData.size() - pos(); }
		string content(const size_type off = 0) const { return theData.size() ? theData.substr(pos() + off) : theData; }
		char peek(const size_type off = 0) const { return theData[pos() + off]; }
		bool atEnd() const { return isAtEnd; }

		void append(const string &aData);
		void atEnd(bool be) { isAtEnd = be; } // may still have data

		void advance(size_type delta);
		void backtrack(size_type delta);

		void reset();

		string parsedContent() const { return theData.substr(0, pos()); }
		size_type parsedSize() const { return pos(); }
		string maxProgress() const { return theData.substr(0, theMaxProgress); }

	protected:
		size_type pos() const;

	protected:
		string theData;
		size_type thePos;
		size_type theMaxProgress;
		bool isAtEnd;
};

} // namespace


#endif
