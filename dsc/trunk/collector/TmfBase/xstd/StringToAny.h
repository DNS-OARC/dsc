 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_LIBS__XSTD_STRING_TO_ANY__H
#define TMF_LIBS__XSTD_STRING_TO_ANY__H

#include <list>
#include "xstd/Time.h"
#include "xstd/NetAddr.h"

extern bool StringToInt(const string &s, int &i, const char **p = 0, int base = 0);
extern bool StringToNum(const string &s, double &d, const char **p = 0);
extern bool StringToDouble(const string &s, double &d, const char **p = 0);
extern Time StringToTime(const string &s);
extern Time StringToDate(const string &s);
extern NetAddr StringToNetAddr(const string &s);

extern string StringToDqsString(const string &s); // double quoted shell string
extern string StringToHtmlString(const string &s); // use &codes;
extern string StringToAnchorString(const string &s); // avoid special chars

extern list<string> StringToList(const string &s, char del);


#endif
