/* This code is for injection into generated test sources */

#include "Hapy/Parser.h"
#include "Hapy/Rules.h"
#include "Hapy/Assert.h"
#include "Hapy/Actions.h"
#include "Hapy/IoStream.h"

using namespace Hapy;

//static void test_action(Action::Params *) {}

struct TestCase {
	Parser *parser;
	string input;
	string *expectedPree;
	string expectedAction;

	TestCase(): parser(0), expectedPree(0) {}
	~TestCase() { delete expectedPree; }

	bool test();
};

static
string PreeLayout(const Pree &p) {
	const Pree::size_type count = p.count();
	if (count == 0)
		return p.image();

	string buf = "(";
	for (Pree::size_type i = 0; i < count; ++i) {
		if (i)
			buf += ',';

		buf += PreeLayout(p[i]);
	}
	buf += ")";

	return buf;
}

bool TestCase::test() {
	const bool didAccept = parser->parse(input);
	const bool shouldAccept = expectedAction == "accept";

	enum { rsUnknown, rsFailed, rsPassed } result = rsUnknown;
	string preeLayout;
	bool hideLayout = false;

	if (didAccept)
		preeLayout = PreeLayout(parser->result().pree);

	if (shouldAccept && didAccept) {
		const string parsedImage = parser->result().pree.rawImage();
		if (parsedImage != input) {
			clog << "\t- failed to " << expectedAction << " '" << input <<
				"' due to parsed image mismatch";
			clog << "\t  expected: " << input << endl;
			clog << "\t  got:      " << parsedImage;
			result = rsFailed;
		}
		if (expectedPree && preeLayout != *expectedPree) {
			clog << "\t- failed to " << expectedAction << " '" << input <<
				"' due to pree layout mismatch:" << endl;
			clog << "\t  expected: " << *expectedPree << endl;
			clog << "\t  got:      " << preeLayout;
			hideLayout = true; // report once only
			result = rsFailed;
		}
	}

	if (result == rsUnknown && didAccept == shouldAccept) {
		clog << "\t+ passed: " << expectedAction << "ed '" << input << "'";
		result = rsPassed;
	}

	if (result == rsUnknown) {
		Should(didAccept != shouldAccept);
		clog << "\t- failed to " << expectedAction << " '" << input << "' " << 
			parser->result().location();
		result = rsFailed;
	}

	if (!hideLayout)
		clog << "  pree={" << preeLayout << "}";

	clog << endl;

	return result == rsPassed;
}
