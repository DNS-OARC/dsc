/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/Assert.h>
#include <Hapy/IoStream.h>

#include <functional>
#include <algorithm>
#include <iomanip>


using namespace Hapy;

RuleId grTests, grTest, grBody, grPreamble, grCode, grGrammar,
	grCases, grCase, grCaseBody, grCasePreeImage,
	grRules, grRule, grName, grComment,
	grToken, grBareToken, grQuotedToken, grEscapedChar, grAnyQuote;

// used to format C++ output
class CppPfx {
	public:
		CppPfx(int aDelta, bool doEndl = false): theDelta(aDelta), doingEndl(doEndl) {}

		string pfx() const {
			TheLevel = TheLevel + theDelta;
			return (doingEndl ? "\n" : "") +
				(TheLevel > 0 ? string(TheLevel*4, ' ') : string());
		}

		ostream &print(ostream &os) const {
			return os << pfx();
		}

	private:
		static int TheLevel;
		int theDelta;
		bool doingEndl;
};

int CppPfx::TheLevel = 0;

static
ostream &operator <<(ostream &os, const CppPfx &pfx) {
	return pfx.print(os);
}

// begin "C++" lines
static
ostream &cppb(int levelDelta = 0) {
	return cout << CppPfx(levelDelta);
}

// end "C++" line(s)
static
ostream &cppe(ostream &os) {
	return os << "\n";
}

// continue "C++" lines
static
ostream &cppc(ostream &os) {
	return os << cppe << CppPfx(0);
}

// continue "C++" lines with a new level
static
CppPfx cppc(int levelDelta) {
	return CppPfx(levelDelta, true);
}

static const string OptShareParsersYes = "--share-parsers";
static const string OptShareParsersNo = "--encapsulate-parsers";
static bool OptShareParsers = false;

static const string OptParseAtOnceYes = "--parse-at-once";
static const string OptParseAtOnceNo = "--parse-incrementally";
static bool OptParseAtOnce = true;

static const string OptHelpYes = "--help";


static
Rule grammar() {
	Rule rTests("Tests", &grTests);
	Rule rTest("Test", &grTest);
	Rule rBody("Body", &grBody);
	Rule rPreamble("Preamble", &grPreamble);
	Rule rCode("Code", &grCode);
	Rule rGrammar("Grammar", &grGrammar);
	Rule rCases("Cases", &grCases);
	Rule rCase("Case", &grCase);
	Rule rCaseBody("CaseBody", &grCaseBody);
	Rule rCasePreeImage("CasePreeImage", &grCasePreeImage);
	Rule rRules("Rules", &grRules);
	Rule rRule("Rule", &grRule);
	Rule rComment("Comment", &grComment);
	Rule rName("Name", &grName);
	Rule rToken("Token", &grToken);
	Rule rBareToken("BareToken", &grBareToken);
	Rule rQuotedToken("QuotedToken", &grQuotedToken);
	Rule rEscapedChar("EscapedChar", &grEscapedChar);
	Rule rAnyQuote("AnyQuote", &grAnyQuote);

	const char sqw = '\'';

	rTests = *rTest;
	rTest = "test" >> rName >> "{" >> rBody >> "}" ;
	rBody = !rPreamble >> rGrammar >> rCases;
	rPreamble = string_r("preamble") >> "=" >> "{{" >> rCode >> "}}";
	rGrammar = string_r("grammar") >> "=" >> "{" >> rRules >> "}";
	rCases = *rCase;
	rCase = rName >> rCaseBody >> !rCasePreeImage;
	rCaseBody = quoted_r(anychar_r, "{", "}");
	rCasePreeImage = string_r("pree") >> "=" >> quoted_r(anychar_r, "{", "}");

	rRules = *rRule;
	rRule = 
		"Rule" >> rName >> "=" >> +rToken >> ";" |
		rName >> "=" >> +rToken >> ";" |
		rName >> "." >> +rToken >> ";";

	rCode =
		*(anychar_r - "}}");

	rComment =
		quoted_r(anychar_r, "//", eol_r) |
		quoted_r(anychar_r, "/*", "*/");

	rName = alpha_r >> *(alnum_r | "_");
	rToken = rBareToken | rQuotedToken;
	rBareToken = +(anychar_r - rAnyQuote - ";");
	rQuotedToken = 
		quoted_r(rEscapedChar | char_r('"'), "'") |
		quoted_r(rEscapedChar | char_r(sqw));
	rEscapedChar =
		(anychar_r - rAnyQuote) | 
		char_r('\\') >> anychar_r;
	rAnyQuote = char_r(sqw) | char_r('"');

	// trimming rules
	rTests.trim(*(space_r | rComment));
	rCaseBody.verbatim(true);
	rToken.verbatim(true);
	rName.verbatim(true);
	rComment.verbatim(true);

	// parse tree shaping rules
	rToken.leaf(true);
	rName.leaf(true);

	// parsing optimization rules
	rToken.committed(true);
	rName.committed(true);
	rTest.committed(true);
	rRule.committed(true);

	return rTests;
}

static
void dumpRuleDecl(const Pree &rule) {
	// skip rules that do not need declaration and non-rule code
	if (rule[0][0].rid() != grName || rule[0][1].image() != "=")
		return;

	const string name = rule[0][0].image();
	//cppb() << "Rule " << name << "(\"" << name << "\", 0);" << cppe;
	cppb() << "Rule " << name << ";" << cppe;
}

static
void dumpParserDecl() {
	cppb() << "Parser parser;" << cppc <<
		"parser.grammar(rGrammar);" << cppc << cppe;
}

static
void dumpCase(const Pree &c) {
	const string &kind = c[0].image();
	const string input = c[1][1].image();
	const string *expectedPree = (c[2].count() > 0) ?
		new string(c[2].find(grCasePreeImage)[2][1].image()) : 0;

	cppb() << "{ // test case" << cppe;
	cppb(+1) << "++caseCount;" << cppc <<
		cppe;

	if (!OptShareParsers)
		dumpParserDecl();

	cppb() << "TestCase tc;" << cppe;
	cppb() << "tc.parser = &parser;" << cppe;
	cppb() << "tc.input = \"" << input << "\";" << cppe;
	cppb() << "tc.expectedAction = \"" << kind << "\";" << cppe;
	if (expectedPree)
		cppb() << "tc.expectedPree = new string(\"" << *expectedPree << "\");" << cppe;

	cppb() << cppe;

	cppb() << "if (tc.test())" << cppc(+1) <<
		"++passCount;" << cppc(-1);

	cppb(-1) << "}" << cppc <<
		cppe;
}

static
void interpPreambles(const Pree &test) {
	const Pree &body = test.find(grBody);
	if (body[0].count() > 0) {
		const string id = test[1].image();
		const Pree &code = body[0].find(grPreamble).find(grCode);

		cppb() << "// from test clause " << id << cppe;
		cppb() << code.image() << cppe;

		cppb() << cppe;
	}
}

static
void interpTest(const Pree &test) {
	const string id = test[1].image();
	cppb() << "{ // test clause " << id << cppe;

	cppb(+1) << "clog << \"test clause: " << id << " {\" << endl;" << cppc <<
		cppe;

	const Pree &rules = test.find(grBody).find(grGrammar).find(grRules);
	for_each(rules.begin(), rules.end(), std::ptr_fun(&dumpRuleDecl));
	cppb() << rules.image() << cppe;

	if (OptShareParsers)
		dumpParserDecl();

	const Pree &cases = test.find(grBody).find(grCases);
	for_each(cases.begin(), cases.end(), std::ptr_fun(&dumpCase));

	cppb() << "clog << \"}\" << endl;" << cppe;

	cppb(-1) << "} // " << id << cppe;

	cppb() << cppe;
}

static
void interpret(const Pree &n) {
	cppb() << "/* autogenerated by Hapy TestMaker */" << cppc <<
		"#include \"TestCase.h\"" << cppc <<
		"" << cppc <<
		cppe;

	for_some(n, grTest, std::ptr_fun(&interpPreambles));

	cppb() << "int main() {" << cppc(+1) <<
			"int caseCount = 0;" << cppc <<
			"int passCount = 0;" << cppc <<
		cppe;

	for_some(n, grTest, std::ptr_fun(&interpTest));

	cppb() << "clog << endl;" << cppe;
	cppb() << "clog << \"test cases: \" << caseCount << endl;" << cppe;
	cppb() << "clog << \"successes:  \" << passCount << endl;" << cppe;
	cppb() << "clog << \"failures:   \" << (caseCount-passCount) << endl;" << cppe;

	cppb() << "return (passCount == caseCount) ? 0 : 255;" << cppc(-1) <<
		"}" << cppe;
}

static
void usage(ostream &os) {
	const int w = 20;
	os << "options:" << endl <<
		std::setw(w) << OptShareParsersYes <<
			"\t: use one parser for all cases in a clause" << endl <<
		std::setw(w) << OptShareParsersNo <<
			"\t: use one parser for each case in a clause" << endl <<
		std::setw(w) << OptParseAtOnceYes <<
			"\t: feed all input at once" << endl <<
		std::setw(w) << OptParseAtOnceNo <<
			"\t: feed all input one octete at a time" << endl <<
		std::setw(w) << OptHelpYes <<
			"\t: this list of options" << endl <<
		endl;
}

static
void configure(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		const string option = argv[i];
		if (option == OptShareParsersYes)
			OptShareParsers = true;
		else
		if (option == OptShareParsersNo)
			OptShareParsers = false;
		else
		if (option == OptParseAtOnceYes)
			OptParseAtOnce = true;
		else
		if (option == OptParseAtOnceNo)
			OptParseAtOnce = false;
		else {
			usage(option == OptHelpYes ? cout : cerr);
			std::exit(option == OptHelpYes ? 0 : 255);
		}
	}
}

int main(int argc, char *argv[]) {
	configure(argc, argv);

	string content;
	char c;
	while (cin.get(c))
		content += c;

	Parser parser;
	parser.grammar(grammar());
	if (!parser.parse(content)) {
		string::size_type p = parser.result().maxPos;
		cerr << "failure after " << p << " bytes near " << content.substr(p).substr(0, 160) << endl;
		return 2;
	}
//cerr << here << "complete success" << endl;
//parser.result().pree.rawPrint(cerr << here << "parsing tree raw" << endl, "");
//parser.result().pree.print(cerr << here << "parsing tree user" << endl, "");

	interpret(parser.result().pree);

	return 0;
}
