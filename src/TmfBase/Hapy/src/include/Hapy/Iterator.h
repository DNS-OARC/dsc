/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_XSTL_ITERATOR__H
#define HAPY_XSTL_ITERATOR__H

#include <Hapy/Top.h>

#ifdef HAPY_HAVE_ITERATOR
#	include <iterator>
#endif

namespace Hapy {

#ifdef HAPY_HAVE_STD_ITERATOR_TYPE
	template <typename ValueT, typename DiffT>
	struct std_bidirectional_iterator:
		public std::iterator<std::bidirectional_iterator_tag, ValueT, DiffT> {
	};
#else
	template <typename ValueT, typename DiffT>
	struct std_bidirectional_iterator:
		public std::bidirectional_iterator<ValueT, DiffT> {
	};
#endif

} // namespace

#endif

