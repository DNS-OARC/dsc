 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <iostream>
#include <iomanip>

#include "xstd/Size.h"


ostream &Size::print(ostream &os) const {
	return os << theSize;
}

ostream &Size::prettyPrint(ostream &os) const {

	if (theSize < 1024) {
		os << theSize << "Bytes";
	} else {
		const int osprec = os.precision();
		os.precision(3);
		
		if (theSize < 1024*1024)
			os << theSize/1024. << "KB";
		else
		if (theSize < 1024*1024*1024)
			os << theSize/(1024*1024.) << "MB";
		else
			os << theSize/(1024*1024*1024.) << "GB";
		
		os.precision(osprec);
	}

	return os;
}
