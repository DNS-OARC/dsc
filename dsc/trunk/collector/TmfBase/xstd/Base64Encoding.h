 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_BASE64_ENCODING__H
#define TMF_BASE__XSTD_BASE64_ENCODING__H

#include <string>

class Base64Encoding {
	public:
		Base64Encoding(const string &data);
		
		string decode() const;
		string encode() const;

	private:
		const string theData;
};

#endif
