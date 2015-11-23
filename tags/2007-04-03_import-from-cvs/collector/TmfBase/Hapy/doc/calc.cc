/* Hapy is a public domain software. See Hapy README file for the details. */

#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/IoStream.h>

#include <functional>

using namespace Hapy;


/* This program, a simple calculator, illustrates basic Hapy parsing
   and interpretation techniques common to parsing available a priory
   input. Compare with calci.cc */


Rule rExpression;
Rule rNumber;
Rule rGrammar;

// extract top-level expressions (less magical version)
int intrpNumber(const Pree &num) {
	return atoi(num.image().c_str());
}

// calculate expression value
int intrpExpr(const Pree &expr) {
	// rExpression = rNumber | parens | sum
	const Pree &alt = expr[0];
	if (alt.rid() == rNumber.id()) // rNumber
		return intrpNumber(alt);
	else
	if (alt[0].image() == "(") // parens
		return intrpExpr(alt[1]);
	else // sum
		return intrpExpr(alt[0]) + intrpExpr(alt[2]);
}

// interpret and report a single top-level expression
void intrpTop(const Pree &expr) {
	const int value = intrpExpr(expr);
	cout << expr.image() << " = " << value << endl;
}

// extract top-level expressions (magical version)
void interpret(const Pree &result) {
	for_some(result, rExpression.id(), std::ptr_fun(&intrpTop));
}

int main() {
	rGrammar = +(rExpression >> ';');
	rExpression = 
		rNumber |
		'(' >> rExpression >> ')' |
		rExpression >> '+' >> rExpression;
	rNumber = digit_r >> *digit_r;

	rGrammar.trim(*space_r);
	rNumber.verbatim(true);

	rGrammar.committed(true);
	rNumber.leaf(true);
	rNumber.committed(true);

	string content;
	char c;
	cin.unsetf(std::ios::skipws);
	while (cin.get(c)) {
		content += c;
	}

	Parser parser;
	parser.grammar(rGrammar);
	if (!parser.parse(content)) {
		cerr << parser.result().location() << ": syntax error" << endl;
		return 2;
	}
	//parser.result().pree.print(cerr << "success; parsing tree:" << endl);

	interpret(parser.result().pree);
	return 0;
}
