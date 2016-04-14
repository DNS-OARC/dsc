/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PARSER__H
#define HAPY_PARSER__H

#include <Hapy/Pree.h>
#include <Hapy/Rules.h>
#include <Hapy/Buffer.h>
#include <Hapy/RuleCompFlags.h>

namespace Hapy {

class Parser {
	public:
		Parser();

		void grammar(const Rule &aStart);

		// simple: parse everything at once
		// returns true iff success
		bool parse(const string &content);

		// advanced: iterative parsing;
		// begin/step/end methods return true iff it makes sense to continue
		bool begin();
		bool step();  
		bool end();
		void moveOn(); // can be called between end() and begin()

		// data supply channel for iterative parsing API
		void pushData(const string &newData);
		bool hasData() const;
		bool sawDataEnd() const;
		void sawDataEnd(bool did);

		// all interfaces
		const Result &result() const;

		// optional grammar determinism check after a successful parse
		bool nextMatch();

	protected:
		bool last();
		bool compile();

	private:
		RulePtr theStartRule;
		RuleCompFlags theCflags;

		Buffer theBuffer;
		Result theResult;

		bool isCompiled;
};

} // namespace

#endif

