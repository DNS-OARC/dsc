 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_FOREIGN_MEM_FUN__H
#define TMF_BASE__XSTD_FOREIGN_MEM_FUN__H

#include <functional>

template <class Fc, class _Ret, class _Tp>
class foreign_mem_fun_t : public unary_function<_Tp,_Ret> {
public:
  explicit foreign_mem_fun_t(Fc *c, _Ret (Fc::*__pf)(_Tp)) : __c(c), _M_f(__pf) {}
  _Ret operator()(_Tp __p) const { return (__c->*_M_f)(__p); }
private:
  Fc *__c;
  _Ret (Fc::*_M_f)(_Tp);
};

template <class Fc, class _Ret, class _Tp>
class foreign_mem_fun_ref_t : public unary_function<_Tp,_Ret> {
public:
  explicit foreign_mem_fun_ref_t(Fc *c, _Ret (Fc::*__pf)(_Tp&)) : __c(c), _M_f(__pf) {}
  _Ret operator()(_Tp& __p) const { return (__c->*_M_f)(__p); }
private:
  Fc *__c;
  _Ret (Fc::*_M_f)(_Tp&);
};

template <class Fc, class _Ret, class _Tp, class Arg>
class foreign_mem_fun1_t : public binary_function<_Tp,Arg,_Ret> {
public:
  explicit foreign_mem_fun1_t(Fc *c, _Ret (Fc::*__pf)(_Tp,Arg)) : __c(c), _M_f(__pf) {}
  _Ret operator()(_Tp __p, Arg a) const { return (__c->*_M_f)(__p, a); }
private:
  Fc *__c;
  _Ret (Fc::*_M_f)(_Tp, Arg);
};

template <class Fc, class _Ret, class _Tp>
class const_foreign_mem_fun_t : public unary_function<_Tp,_Ret> {
public:
  explicit const_foreign_mem_fun_t(Fc *c, _Ret (Fc::*__pf)(_Tp) const) : __c(c), _M_f(__pf) {}
  _Ret operator()(_Tp __p) const { return (__c->*_M_f)(__p); }
private:
  Fc *__c;
  _Ret (Fc::*_M_f)(_Tp) const;
};

template <class Fc, class _Ret, class _Tp>
class const_foreign_mem_fun_ref_t : public unary_function<_Tp,_Ret> {
public:
  explicit const_foreign_mem_fun_ref_t(Fc *c, _Ret (Fc::*__pf)(_Tp&) const) : __c(c), _M_f(__pf) {}
  _Ret operator()(_Tp& __p) const { return (__c->*_M_f)(__p); }
private:
  Fc *__c;
  _Ret (Fc::*_M_f)(_Tp&) const;
};

template <class Fc, class _Ret, class _Tp, class Arg>
class const_foreign_mem_fun1_t : public binary_function<_Tp,Arg,_Ret> {
public:
  explicit const_foreign_mem_fun1_t(Fc *c, _Ret (Fc::*__pf)(_Tp,Arg) const) : __c(c), _M_f(__pf) {}
  _Ret operator()(_Tp __p, Arg a) const { return (__c->*_M_f)(__p, a); }
private:
  Fc *__c;
  _Ret (Fc::*_M_f)(_Tp, Arg) const;
};



template <class Fc, class _Ret, class _Tp>
inline foreign_mem_fun_t<Fc,_Ret,_Tp> foreign_mem_fun(Fc *c, _Ret (Fc::*__f)(_Tp)) {
	return foreign_mem_fun_t<Fc,_Ret,_Tp>(c, __f);
}

template <class Fc, class _Ret, class _Tp>
inline foreign_mem_fun_ref_t<Fc,_Ret,_Tp> foreign_mem_fun_ref(Fc *c, _Ret (Fc::*__f)(_Tp&)) {
	return foreign_mem_fun_ref_t<Fc,_Ret,_Tp>(c, __f);
}

template <class Fc, class _Ret, class _Tp, class Arg>
inline foreign_mem_fun1_t<Fc,_Ret,_Tp,Arg> foreign_mem_fun(Fc *c, _Ret (Fc::*__f)(_Tp,Arg)) {
	return foreign_mem_fun1_t<Fc,_Ret,_Tp,Arg>(c, __f);
}

template <class Fc, class _Ret, class _Tp>
inline const_foreign_mem_fun_t<Fc,_Ret,_Tp> foreign_mem_fun(Fc *c, _Ret (Fc::*__f)(_Tp) const) {
	return const_foreign_mem_fun_t<Fc,_Ret,_Tp>(c, __f);
}

template <class Fc, class _Ret, class _Tp>
inline const_foreign_mem_fun_ref_t<Fc,_Ret,_Tp> foreign_mem_fun_ref(Fc *c, _Ret (Fc::*__f)(_Tp&) const) {
	return const_foreign_mem_fun_ref_t<Fc,_Ret,_Tp>(c, __f);
}

template <class Fc, class _Ret, class _Tp, class Arg>
inline const_foreign_mem_fun1_t<Fc,_Ret,_Tp,Arg> foreign_mem_fun(Fc *c, _Ret (Fc::*__f)(_Tp,Arg) const) {
	return const_foreign_mem_fun1_t<Fc,_Ret,_Tp,Arg>(c, __f);
}

#endif
