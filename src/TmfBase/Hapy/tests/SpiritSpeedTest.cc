#define BOOST_DISABLE_THREADS 1

#include "boost/spirit/core.hpp"
#include "boost/spirit/tree/parse_tree.hpp"
#include "boost/spirit/tree/ast.hpp"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <iostream>
#include <stack>
#include <functional>
#include <string>

using namespace std;
using namespace boost::spirit;

// Here's some typedefs to simplify things
typedef char const*         iterator_t;
typedef tree_match<iterator_t> parse_tree_match_t;
typedef parse_tree_match_t::const_tree_iterator iter_t;

typedef pt_match_policy<iterator_t> match_policy_t;
typedef rule<phrase_scanner_t> rule_t;


class XmlGrammar: public boost::spirit::grammar<XmlGrammar> {
	public:
		XmlGrammar() {}

	public:
		template <typename ScannerT>

		struct definition {

			definition(const XmlGrammar &self); 

			typedef rule<ScannerT> rule_t;

			rule<ScannerT> const &start() const { return rGrammar; }

			rule_t rGrammar;
			rule_t rNode;
			rule_t rPi;
			rule_t rElement;
			rule_t rOpenElement;
			rule_t rCloseElement;
			rule_t rClosedElement;
			rule_t rText;
			rule_t rAttr;
			rule_t rName;
			rule_t rValue;

		};
};


template <typename ScannerT>
inline
XmlGrammar::definition<ScannerT>::definition(const XmlGrammar &self) {
	rGrammar = *rNode;
	rNode = rPi | rElement | rText;

	rPi = "<?" >> rName >> *(anychar_p - "?>") >> "?>";

	rElement = rOpenElement >> *rNode >> rCloseElement | rClosedElement;
	rOpenElement = ch_p('<') >> rName >> *rAttr >> ch_p('>');
	rCloseElement = str_p("</") >> rName >> ch_p('>');
	rClosedElement = ch_p('<') >> rName >> *rAttr >> str_p("/>");

	rText = +(anychar_p - ch_p('<'));

	rAttr = rName >> "=" >> rValue;
	rName = lexeme_d[ token_node_d[
		alpha_p >> *(alnum_p | ch_p('_') | ch_p(':'))
	]];
	rValue = lexeme_d[ token_node_d[
		ch_p('"') >> *(anychar_p - ch_p('"')) >> ch_p('"')
	]];
}


unsigned int calibrate(const string &input) {
	unsigned int sum = input.size();
	for (string::const_iterator i = input.begin(); i != input.end(); ++i) {
		const unsigned int lastBit = sum >> (8*sizeof(sum)-1);
		sum = ((sum << 1) | lastBit) ^ (unsigned char)*i;
	}
	return sum;
}

timeval operator -(const timeval &t1, const timeval &t2) {
	timeval res(t1);
	res.tv_sec -= t2.tv_sec;
	if (res.tv_usec >= t2.tv_usec) {
		res.tv_usec -= t2.tv_usec;
	} else {
		--res.tv_sec;
		res.tv_usec = 1000000 - (t2.tv_usec - res.tv_usec);
	}
	return res;
}

timeval operator +(const timeval &t1, const timeval &t2) {
	timeval res(t1);
	res.tv_sec += t2.tv_sec;
	res.tv_usec += t2.tv_usec;
	return res;
}

ostream &operator <<(ostream &os, const timeval &t) {
	return os << (t.tv_sec + t.tv_usec/1e6);
}

rusage operator -(const rusage &r1, const rusage &r2) {
	rusage res(r1);
	res.ru_utime = r1.ru_utime - r2.ru_utime;
	res.ru_stime = r1.ru_stime - r2.ru_stime;
	res.ru_maxrss = r1.ru_maxrss - r2.ru_maxrss;
	return res;
}

rusage startMeasurements() {
	struct rusage cpStart;
	getrusage(RUSAGE_SELF, &cpStart);
	return cpStart;
}

rusage stopMeasurements(const rusage &cpStart) {
	struct rusage cpEnd;
	getrusage(RUSAGE_SELF, &cpEnd);

	const struct rusage cpDiff = cpEnd - cpStart;
	return cpDiff;
}

double allTimes(const rusage &r) {
	const timeval t = r.ru_utime + r.ru_stime;
	return t.tv_sec + t.tv_usec/1e6;
}

void configureStream(ostream &os, int prec) {
	using std::ios;
	os.precision(prec);
	os.setf(ios::fixed, ios::floatfield);
	os.setf(ios::showpoint, ios::basefield);
}

int main() {
	configureStream(cout, 3);

	clog << "reading ... ";
	string input;
	char c;
	cin.unsetf(std::ios::skipws);
	while (cin.get(c))
		input += c;
	clog << "done" << endl;

	cout << "input:  " << (input.size()/(1024.)) << " KB" << endl;

	const rusage cpCalibrStart = startMeasurements();
	const unsigned int checksum = calibrate(input);
	const rusage cpCalibr = stopMeasurements(cpCalibrStart);
	cout << "checksum: " << checksum << endl;

	struct rusage ruStart;
	getrusage(RUSAGE_SELF, &ruStart);

    const char* first = input.c_str();
	const rusage cpParserStart = startMeasurements();
	XmlGrammar g;
	//parse_info<> info = parse(first, g, space_p);
	tree_parse_info<> info = ast_parse(first, g, space_p);
	const rusage cpParser = stopMeasurements(cpParserStart);
	if (!info.full) {
		cerr << "syntax error" << endl;
		return 2;
	}

	cout <<
		"time:   " << allTimes(cpParser) << " sec (" <<
			cpParser.ru_utime << " + " << cpParser.ru_stime << ")" << endl <<
		"memory: " << (cpParser.ru_maxrss/1024.) << " MB" << endl;

	return 0;
}


// g++ -Wall -O3 -o SpiritSpeedTest -ftemplate-depth-54 SpiritSpeedTest.cc -I /usr/local/include
