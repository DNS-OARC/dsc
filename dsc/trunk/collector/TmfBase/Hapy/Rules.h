 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_RULES__H
#define TMF_BASE__HAPY_RULES__H

#include "Hapy/Rule.h"

namespace Hapy {

extern Rule operator !(const Rule &r); // optional
extern Rule operator |(const Rule &a, const Rule &b);
extern Rule operator -(const Rule &a, const Rule &b);
extern Rule operator >>(const Rule &a, const Rule &b);
extern Rule operator *(const Rule &r);
extern Rule operator +(const Rule &r);

extern Rule empty_r();
extern Rule anychar_r();
extern Rule char_r(char c);
extern Rule string_r(const string &s);
extern Rule alpha_r();
extern Rule digit_r();
extern Rule alnum_r();
extern Rule space_r();
extern Rule eol_r();
extern Rule quoted_r(char open, const Rule &middle, char close);
extern Rule quoted_r(const Rule &open, const Rule &middle, const Rule &close);
extern Rule commit_r(const Rule &r);
extern Rule aggregate_r(const Rule &r);
extern Rule end_r();

} // namespace


#endif
