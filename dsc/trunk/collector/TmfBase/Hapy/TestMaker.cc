 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include "Hapy/Parser.h"
#include "Hapy/Rules.h"
#include "xstd/Assert.h"

using namespace Hapy;

int main() {
	int id = 0;
	Rule rTests("Tests", ++id);
	Rule rTest("Test", ++id);
	Rule rBody("Body", ++id);
	Rule rCases("Cases", ++id);
	Rule rCase("Case", ++id);
	Rule rRules("Rules", ++id);
	Rule rRule("Rule", ++id);
	Rule rName("Name", ++id);
	Rule rToken("Token", ++id);
	Rule rBareToken("BareToken", ++id);
	Rule rQuotedToken("QuotedToken", ++id);
	Rule rEscapedChar("EscapedChar", ++id);
	Rule rAnyQuote("AnyQuote", ++id);

	const char sqw = '\'';

	rTests = *rTest >> end_r();
	rTest = "test" >> rName >> "{" >> rBody >> "}" ;
	rBody = rName >> "="  >> "{" >> rRules >> "}" >> rCases;
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

	string content;
	char c;
	while (cin.get(c))
		content += c;

	Parser parser;
	parser.grammar(rTests);
	if (!parser.parse(content)) {
		string::size_type p = /* XXX: pree.pos + */ parser.result().pree.rawImageSize();
cerr << here << "failure after " << p << " bytes near " << content.substr(p).substr(0, 160) << endl;
		return 2;
	}
cerr << here << "complete success" << endl;
parser.result().pree.rawPrint(cerr << here << "parsing tree raw" << endl, "");
parser.result().pree.print(cerr << here << "parsing tree user" << endl, "");
	return 0;
}
