/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULES__H
#define HAPY_RULES__H

#include <Hapy/Rule.h>

namespace Hapy {

extern Rule operator !(const Rule &r);
extern Rule operator |(const Rule &a, const Rule &b);
extern Rule operator -(const Rule &a, const Rule &b);
extern Rule operator >>(const Rule &a, const Rule &b);
extern Rule operator *(const Rule &r);
extern Rule operator +(const Rule &r);

extern const Rule empty_r;
extern const Rule anychar_r;
extern const Rule alpha_r;
extern const Rule digit_r;
extern const Rule alnum_r;
extern const Rule space_r;
extern const Rule eol_r;
extern const Rule end_r;

extern Rule char_r(char c);
extern Rule char_set_r(const string &s);
extern Rule char_range_r(char first, char last);

extern Rule string_r(const string &s);

extern Rule quoted_r(const Rule &element);
extern Rule quoted_r(const Rule &element, const Rule &open);
extern Rule quoted_r(const Rule &element, const Rule &open, const Rule &close);

} // namespace


#endif
