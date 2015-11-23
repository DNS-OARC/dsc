/* Hapy is a public domain software. See Hapy README file for the details. */

/* This program, a simple "interactive" calculator, illustrates some of
   the techniques common to parsing a stream of "messages". In this case,
   a message is an arithmetic expression. Compare with calc.cc */

#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/IoStream.h>

#include <iterator>
#include <functional>

using namespace Hapy;

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

// supply parser with more input
void getData(Parser &parser) {
	if (!parser.sawDataEnd()) {
		char c;
		if (cin.get(c))
			parser.pushData(string(1, c));
		else
			parser.sawDataEnd(true);
	}
}

// interpret one expression; return true if we should continue
bool interpret(const Result &result) {
	if (result.statusCode == Result::scMatch) {
		intrpTop(result.pree.find(rExpression.id()));
		return true;
	}

	// if empty input did not parse, that is OK; just stop
	if (result.input.size() > 0)
		cerr << result.location() << ": syntax error" << endl;
	return false;
}

void interpretEach(Parser &parser) {
	do {
		parser.moveOn();
		if (parser.begin()) {
			while (parser.step())
				getData(parser);
		}
		parser.end();
	} while (interpret(parser.result()));
}

int main() {
	rGrammar = rExpression >> ';';
	rExpression = 
		rNumber |
		'(' >> rExpression >> ')' |
		rExpression >> '+' >> rExpression;
	rNumber = +digit_r;

	rExpression.trim(*space_r);
	rNumber.verbatim(true);

	rGrammar.committed(true);
	rNumber.leaf(true);
	rNumber.committed(true);

	Parser parser;
	parser.grammar(rGrammar);

	cin.unsetf(std::ios::skipws);
	interpretEach(parser);

	return 0;
}
