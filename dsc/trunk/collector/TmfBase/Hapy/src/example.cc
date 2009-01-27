/* Hapy is a public domain software. See Hapy README file for the details. */


#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/IoStream.h>

using namespace Hapy;

int main() {
	Rule rName;
	Rule rToken;
	Rule rInstr;
	Rule rStatement;
	Rule rCode;

	rCode = *rStatement;
	rStatement = *rInstr >> ';';
	rInstr = +rToken;
	rToken = alpha_r >> *(alpha_r | digit_r | '_' | '.');

	rCode.trim(*space_r);
	rToken.verbatim(true);
	rToken.leaf(true);

	string content;
	char c;
	while (cin.get(c)) {
		content += c;
	}

	Parser parser;
	parser.grammar(rCode);
	if (!parser.parse(content)) {
		cerr << parser.result().location() << ": syntax error" << endl;
		return 2;
	}

	parser.result().pree.print(cout << "success; parsing tree:" << endl);
	return 0;
}
