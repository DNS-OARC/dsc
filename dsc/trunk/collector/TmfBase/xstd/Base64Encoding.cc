 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include "xstd/Base64Encoding.h"

static bool Base64Encoding_initialized = false;
static int Base64Encoding_value[256];
const char Base64Encoding_code[] = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz0123456789+/";

static
void Base64Encoding_init() {
	const int count = 
		(int)(sizeof(Base64Encoding_value)/sizeof(*Base64Encoding_value));
	{for (int i = 0; i < count; ++i)
		Base64Encoding_value[i] = -1;
	}

	{for (int i = 0; i < 64; i++)
		Base64Encoding_value[(int) Base64Encoding_code[i]] = i;
	}
	Base64Encoding_value[(int)'='] = 0;

	Base64Encoding_initialized = true;
}


Base64Encoding::Base64Encoding(const string &data): theData(data) {
}

// adopted from ftp://ftp.sunet.se/pubh/mac/netatalk/var/ftp/unix/netatalk/archive/netatalk-admins-mail/att-0393/01-base64-encode.c
string Base64Encoding::encode() const {
	if (!Base64Encoding_initialized)
		Base64Encoding_init();

	const char *buf = theData.data();
	string::size_type len = theData.size();
	string res;

	int bits = 0;
	int char_count = 0;
	while (len--) {
		const int c = (unsigned char)*buf++;
		bits += c;
		char_count++;
		if (char_count == 3) {
			res += Base64Encoding_code[bits >> 18];
			res += Base64Encoding_code[(bits >> 12) & 0x3f];
			res += Base64Encoding_code[(bits >> 6) & 0x3f];
			res += Base64Encoding_code[bits & 0x3f];
			bits = 0;
			char_count = 0;
		} else {
			bits <<= 8;
		}
	}

	if (char_count) {
		bits <<= 16 - (8 * char_count);
		res += Base64Encoding_code[bits >> 18];
		res += Base64Encoding_code[(bits >> 12) & 0x3f];
		if (char_count == 1) {
			res += '=';
			res += '=';
		} else {
			res += Base64Encoding_code[(bits >> 6) & 0x3f];
			res += '=';
		}
	}

	return res;
}

		
// XXX: should check for garbage at the end of inBuf if we ran out of outBuf
string Base64Encoding::decode() const {
	if (!Base64Encoding_initialized)
		Base64Encoding_init();

	const char *inBuf = theData.data();
	const string::size_type inLen = theData.size();
	string res;

	int c = 0;
	long val = 0;
	for (string::size_type inPos = 0; inPos < inLen; ++inPos) {
		unsigned int k = ((unsigned int) (unsigned char) inBuf[inPos]) % 256;
		if (Base64Encoding_value[k] < 0)
			continue;
		val <<= 6;
		val += Base64Encoding_value[k];
		if (++c < 4)
			continue;

		res += (char)(val >> 16);          // high 8 bits
		res += (char)((val >> 8) & 0xff);  // middle 8 bits
		res += (char)(val & 0xff);         // low 8 bits
		val = c = 0;
	}

	return res;
}
