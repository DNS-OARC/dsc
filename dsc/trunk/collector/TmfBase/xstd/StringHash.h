 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_STRING_HASH__H
#define TMF_BASE__XSTD_STRING_HASH__H

#include <string>
#include <hash_map>

#ifdef __GNUC__
	// http://gcc.gnu.org/ml/libstdc++/2000-11/msg00122.html
	__STL_TEMPLATE_NULL struct hash<string> {
		size_t operator()(const string &s) const {
			return __stl_hash_string(s.c_str());
		}
	};
#endif

#endif
