 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "config.h"

#include "Hapy/Parser.h"
#include "Hapy/Rules.h"
#include "xstd/Assert.h"

using namespace Hapy;

int main() {
	Rule rName("name", 1);
	Rule rToken("token", 2);
	Rule rInstr("instr", 3);
	Rule rStatement("statement", 4);
	Rule rCode("code", 5);

	rCode = *rStatement >> end_r();
	rStatement = *rInstr >> ";" >> *space_r();
	rInstr = rName | rToken | space_r() >> *space_r();
	rName = string_r("description");
	rToken = alpha_r() >> *(alpha_r() | digit_r() | "_" | ".");

	//rCode = *anychar_r() >> alpha_r() >> "=" >> alpha_r() >> end_r();

	rCode.trim(*space_r());

	string content;
	char c;
	while (cin.get(c)) {
		content += c;
	}

	Parser parser;
	parser.grammar(rCode);
	if (!parser.parse(content)) {
		string::size_type p = /* XXX: pree.pos + */ parser.result().pree.rawImageSize();
cerr << here << "failure after " << p << " bytes near " << content.substr(p).substr(0, 160) << endl;
		return 2;
	}
parser.result().pree.print(cerr << here << "complete success" << endl, "");
	return 0;
}
