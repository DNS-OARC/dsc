 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <algorithm>
#include <functional>

#include "xstd/Assert.h"
#include "xstd/Bind2Ref.h"
#include "xstd/StringToAny.h"


bool StringToInt(const string &str, int &i, const char **p = 0, int base = 0) {
	if (const char *s = str.c_str()) {
		char *ptr = 0;
		const int h = (int) strtol(s, &ptr, base);
		if (ptr != s && ptr) {
			i = h;
			if (p)
				*p = ptr;
			return true;
		}
	}
	return false;
}

bool StringToNum(const string &str, double &d, const char **p) {
	if (const char *s = str.c_str()) {
		char *ptr = 0;
		const double h = strtod(s, &ptr);
		if (ptr != s) {
			d = h;
			if (p) *p = ptr;
			return true;
		}
	}
	return false;
}

bool StringToDouble(const string &str, double &d, const char **p) {
	double dd;
	const char *pp;
	if (!StringToNum(str, dd, &pp))
		return false; 

	Assert(pp);

	// check that it is not an integer (has '.' or 'e')
	const char *s = str.c_str();
	const char *h = strchr(s, '.');
	bool dbl = h && h < pp;
	if (!dbl) {
		h = strchr(s, 'e');
		dbl = h && h < pp;
	}

	if (dbl) {
		d = dd;
		if (p) *p = pp;
	}
	return dbl;
}


Time StringToTime(const string &str) {
	// note: things are set up so that "sec" stands for "1sec" 
	//       (convenient for rates)
	const char *ptr = str.c_str();
	int i = 1;
	double d;

	// try to preserve integer precision if possible
	const bool isi = !StringToDouble(str.c_str(), d, &ptr);

	if (isi)
		StringToInt(str.c_str(), i, &ptr);

	if (!*ptr && isi && i == 0)
		return Time(0,0);
	else
	if (!strcmp(ptr, "msec") || !strcmp(ptr, "ms"))
		return isi ? Time::Msec(i) : Time::Secd(d/1e3);
	else
	if (!strcmp(ptr, "sec") || !strcmp(ptr, "s"))
		return isi ? Time::Sec(i) : Time::Secd(d);
	else
	if (!strcmp(ptr, "min"))
		return isi ? Time::Sec(i*60) : Time::Mind(d);
	else
	if (!strcmp(ptr, "hour") || !strcmp(ptr, "hr"))
		return isi ? Time::Sec(i*60*60) : Time::Hourd(d);
	else
	if (!strcmp(ptr, "day"))
		return isi ? Time::Sec(i*24*60*60) : Time::Dayd(d);
	else
	if (!strcmp(ptr, "year"))
		return isi ? Time::Sec(i*365*24*60*60) : Time::Yeard(d);
	else
		return Time();
}

Time StringToDate(const string &str) {
	int sec;
	const char *beg = str.c_str();
	const char *end = 0;
	if (StringToInt(beg, sec, &end)) {
		if (beg + str.size() == end)
			return Time::Sec(sec);
		if (end && *end == '.') {
			double secd;
			if (StringToDouble(beg, secd, &end) && beg + str.size() == end)
				return Time::Secd(secd);
		}
	}
	return Time();	
}

static
bool StringSplitNetAddr(const string &str, string &ifname, string &addr, int &subnet) {
	string val = str;

	string parsedIfname;
	int parsedSubnet = -1;

	string::size_type pos = val.find("::");
	if (pos != string::npos) { // ifname
		parsedIfname = val.substr(0, pos);
		val = val.substr(pos+2);
	}

	pos = val.find('/');
	if (pos != string::npos) { // subnet
		if (!StringToInt(val.c_str() + pos+1, parsedSubnet))
			return false;
		if (!(1 <= parsedSubnet && parsedSubnet <= 32))
			return false;
		val = val.substr(0, pos);
	}

	// what is left must be a host name (IP or dname) or name range
	// possibly with a port or port range
	if (val.empty())
		return false;

	// check that all characters are valid
	for (const char *c = val.c_str(); *c; ++c) {
		if (!isalpha(*c) && !isdigit(*c) && 
			*c != '-' && *c != '[' && *c != ']' && 
			*c != ':' && *c != '.')
			return false;
		}
		

	ifname = parsedIfname;
	addr = val;
	subnet = parsedSubnet;
	return true;
}

static
bool StringSplitNetAddr(const string &str, string &ifname, string &addr, int &port, int &subnet) {
	string val = str;

	string parsedIfname;
	int parsedPort = -1;
	int parsedSubnet = -1;

	if (!StringSplitNetAddr(str, parsedIfname, val, parsedSubnet))
		return false;

	const string::size_type pos = val.find(':');
	if (pos != string::npos) { // port
		if (!StringToInt(val.c_str() + (pos+1), parsedPort))
			return false;
		val	= val.substr(0, pos);
	}

	// what is left must be a host name (IP or dname) or name range
	if (val.empty())
		return false;

	ifname = parsedIfname;
	addr = val;
	port = parsedPort;
	subnet = parsedSubnet;
	return true;
}

NetAddr StringToNetAddr(const string &s) {
	string ifname;
	string host;
	int port = -1;
	int subnet = -1;

	if (!StringSplitNetAddr(s, ifname, host, port, subnet))
		return NetAddr();

	return NetAddr(host, port);
}

static
void StringToAny_EscapeShellChar(char c, string &s) {
	switch (c) {
		case '"':
		case '$':
		case '`':
		case '\\':
			s += '\\';
			s += c;
			return;
		default:
			s += c;
	}
}

string StringToDqsString(const string &s) {
	string res = "\"";
	for_each(s.begin(), s.end(),
		bind2ref(ptr_fun(&StringToAny_EscapeShellChar), res));
	res += "\"";
	return res;
}

static
void StringToAny_EscapeHtmlChar(char c, string &s) {
	if (c == '"')
		s += "&quot;";
	else
	if (c == '<')
		s += "&lt;";
	else
	if (c == '>')
		s += "&gt;";
	else
	if (c == '&')
		s += "&amp;";
	else
		s += c;
}

string StringToHtmlString(const string &s) {
	string res;
	for_each(s.begin(), s.end(),
		bind2ref(ptr_fun(&StringToAny_EscapeHtmlChar), res));
	return res;
}

static
void StringToAny_HandleAnchorChar(char c, string &s) {
	// encode some characters with special meaning using custom encoding
	// that should be safe for anchors; browsers may not handle these
	// characters in anchors and may not handle URL encodings in anchors
	// either
	if (c == '/')
		s += "_";
	else
	if (c == ';')
		s += "_s_";
	else
	if (c == ':')
		s += "_c_";
	else
	if (c == '@')
		s += "_a_";
	else
	if (c == '_') // not a special meaning character, but used above
		s += "__";
	else
		s += c; // StringToAny_EscapeUrlChar(c);
}

string StringToAnchorString(const string &s) {
	string res;
	for_each(s.begin(), s.end(),
		bind2ref(ptr_fun(&StringToAny_HandleAnchorChar), res));

	return res; 
}

list<string> StringToList(const string &s, char del) {
	list<string> res;
	string val = s;
	while (val.size()) {
		string::size_type pos = val.find(del);
		res.push_back(val.substr(0, pos));
		if (pos == string::npos)
			val = string();
		else
			val = val.substr(pos+1);
	}
	return res;
}
