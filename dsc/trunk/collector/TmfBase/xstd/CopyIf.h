 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_COPY_IF__H
#define TMF_BASE__XSTD_COPY_IF__H


#include <functional>

template <class In, class Out, class Pred>
inline
Out copy_if(In first, In last, Out res, Pred p) {
	while (first != last) {
		if (p(*first)) {
			*res = *first;
			++res;
		}
		++first;
	}
	return res;
}

#endif
