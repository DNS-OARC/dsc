 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_TRANSFORM_IF__H
#define TMF_BASE__XSTD_TRANSFORM_IF__H


#include <functional>

template <class In, class Out, class UnaryOperation, class Pred>
inline
Out transform_if(In first, In last, Out res, UnaryOperation o, Pred p) {
	while (first != last) {
		if (p(*first))
			*res++ = o(*first);
		++first;
	}
	return res;
}

#endif
