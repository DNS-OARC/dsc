 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_BINDER_2_REF__H
#define TMF_BASE__XSTD_BINDER_2_REF__H

#include <functional>

// bind2ref below is the same as bind2nd except the 2nd argument can
// be a reference such as "iostream &"; this avoids error like this:
// /usr/include/g++/stl_function.h:221: forming reference to reference type


template <class _Operation> 
class binder2ref:
		public unary_function<typename _Operation::first_argument_type,
                          typename _Operation::result_type> {
	protected:
		_Operation op;
		typename _Operation::second_argument_type value;

	public:
		binder2ref(const _Operation& __x, const typename _Operation::second_argument_type __y): op(__x), value(__y) {}

		typename _Operation::result_type
		operator()(const typename _Operation::first_argument_type __x) const {
				return op(__x, value); 
		}
};

template <class _Operation, class _Tp>
inline
binder2ref<_Operation> bind2ref(const _Operation& __oper, _Tp &__x) {
  typedef typename _Operation::second_argument_type _Arg2_type;
  return binder2ref<_Operation>(__oper, (_Arg2_type)__x);
}

#endif
