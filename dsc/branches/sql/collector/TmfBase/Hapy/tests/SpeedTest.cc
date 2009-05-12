/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/PreeFarm.h>
#include <Hapy/Assert.h>
#include <Hapy/IoStream.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <functional>
#include <algorithm>


using namespace Hapy;

static
Rule grammar() {
	Rule rGrammar("grammar", 0);
	Rule rNode("node", 0);
	Rule rPi("pi", 0);
	Rule rElement("element", 0);
	Rule rOpenElement("open-element", 0);
	Rule rCloseElement("close-element", 0);
	Rule rClosedElement("closed-element", 0);
	Rule rText("text", 0);
	Rule rAttr("attr", 0);
	Rule rName("name", 0);
	Rule rValue("value", 0);

	rGrammar = *rNode;
	rNode = rElement | rText | rPi;
	rElement = rOpenElement >> *rNode >> rCloseElement | rClosedElement;

	rPi = "<?" >> rName >> *(anychar_r - "?>") >> "?>";

	rOpenElement = "<" >> rName >> *rAttr >> ">";
	rCloseElement = "</" >> rName >> ">";
	rClosedElement = "<" >> rName >> *rAttr >> "/>";

	rText = +(anychar_r - '<');

	rAttr = rName >> '=' >> rValue;
	rName = alpha_r >> *(alnum_r | '_' | ':');
	rValue = quoted_r(anychar_r);

	// trimming rules
	rGrammar.trim(*space_r);
	rText.verbatim(true);
	rName.verbatim(true);
	rValue.verbatim(true);

	// parse tree shaping rules
	rText.leaf(true);
	rName.leaf(true);
	rValue.leaf(true);

	// parsing optimization rules
	rGrammar.committed(true);
	rText.committed(true);
	rName.committed(true);
	rValue.committed(true);
	rNode.committed(true);

	return rGrammar;
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
	Assert(getrusage(RUSAGE_SELF, &cpStart) == 0);
	return cpStart;
}

rusage stopMeasurements(const rusage &cpStart) {
	struct rusage cpEnd;
	Assert(getrusage(RUSAGE_SELF, &cpEnd) == 0);

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
	Assert(getrusage(RUSAGE_SELF, &ruStart) == 0);

	const rusage cpParserStart = startMeasurements();
	Parser parser;
	parser.grammar(grammar());
	const bool success = parser.parse(input);
	const rusage cpParser = stopMeasurements(cpParserStart);
	
	if (!success) {
		cerr << parser.result().location() << ": syntax error" << endl;
		return 2;
	}

	const Pree &pree = parser.result().pree;
	cout <<
		"time:   " << allTimes(cpParser) << " sec (" <<
			cpParser.ru_utime << " + " << cpParser.ru_stime << ")" << endl <<
		"memory: " << (cpParser.ru_maxrss/1024.) << " MB" << endl;

	cout << "prees:  " << pree.rawDeepCount() << " (" <<
			PreeFarm::TheNewCount << " allocated during " <<
			PreeFarm::TheGetCount << " gets and " <<
			PreeFarm::ThePutCount << " puts)" <<
		endl;
	return 0;
}
