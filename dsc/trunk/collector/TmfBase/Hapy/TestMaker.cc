 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include <fstream>
#include <functional>
#include <algorithm>

#include "Hapy/Parser.h"
#include "Hapy/Rules.h"
#include "xstd/Assert.h"

using namespace Hapy;

enum { grTests, grTest, grBody, grCases, grCase, grRules, grRule, grName,
	grToken, grBareToken, grQuotedToken, grEscapedChar, grAnyQuote };

static
Rule grammar() {
	Rule rTests("Tests", grTests);
	Rule rTest("Test", grTest);
	Rule rBody("Body", grBody);
	Rule rCases("Cases", grCases);
	Rule rCase("Case", grCase);
	Rule rRules("Rules", grRules);
	Rule rRule("Rule", grRule);
	Rule rName("Name", grName);
	Rule rToken("Token", grToken);
	Rule rBareToken("BareToken", grBareToken);
	Rule rQuotedToken("QuotedToken", grQuotedToken);
	Rule rEscapedChar("EscapedChar", grEscapedChar);
	Rule rAnyQuote("AnyQuote", grAnyQuote);

	const char sqw = '\'';

	rTests = *rTest >> end_r();
	rTest = "test" >> rName >> "{" >> rBody >> "}" ;
	rBody = string_r("grammar") >> "=" >> "{" >> rRules >> "}" >> rCases;
	rCases = *rCase;
	rCase = rName >> quoted_r('{', anychar_r(), '}');
	rRules = *rRule;
	rRule = 
		rName >> "=" >> +rToken >> ";" |
		rName >> "." >> +rToken >> ";";

	rName = alpha_r() >> *(alnum_r() | "_");
	rToken = rBareToken | rQuotedToken;
	rBareToken = +(anychar_r() - rAnyQuote - ";");
	rQuotedToken = 
		quoted_r(sqw, rEscapedChar | char_r('"'), sqw) |
		quoted_r('"', rEscapedChar | char_r(sqw), '"');
	rEscapedChar =
		(anychar_r() - rAnyQuote) | 
		char_r('\\') >> anychar_r();
	rAnyQuote = char_r(sqw) | char_r('"');

	// trimming rules
	rTests.trim(*space_r());
	rToken.verbatim(true);
	rName.verbatim(true);

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

static ofstream of;

static
void dumpRuleDecl(const PreeNode &rule) {
	// skip non-assignments to avoid repetitions of rules
	if (rule[1].image() != "=")
		return;

	const string name = rule[0].image();
	of << "\t" << name << "(\"" << name << "\", ++id);" << endl;
}

static
void interpTest(const PreeNode &test) {
	const string id = test[1].image();

	const PreeNode rules = test.find(grBody).find(grRules);

	const string testHead = "int main() {";
	const string testTail = "return 0; }";

	const string fname = "test-n.cc";
	of.open(fname.c_str(), ios::out);
	of << "// auto-generated hapy test case (" << id << ")" << endl;
	of << testHead;

	for_each(rules.begin(), rules.end(), ptr_fun(&dumpRuleDecl));
	of << rules.image() << endl;

	of << testTail;
	of.close();

	system(("cat " + fname).c_str());
}

static
void interpret(const PreeNode &n) {
	for_some(n, grTest, ptr_fun(&interpTest));
}

int main() {
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
cerr << here << "complete success" << endl;
parser.result().pree.rawPrint(cerr << here << "parsing tree raw" << endl, "");
parser.result().pree.print(cerr << here << "parsing tree user" << endl, "");

	interpret(parser.result().pree);

	return 0;
}
