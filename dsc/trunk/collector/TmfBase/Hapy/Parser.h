 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__HAPY_PARSER__H
#define TMF_BASE__HAPY_PARSER__H

#include "Hapy/Pree.h"
#include "Hapy/Rules.h"
#include "Hapy/Buffer.h"

namespace Hapy {

class Parser {
	public:
		Parser();

		void grammar(const Rule &aStart);

		// simple: parse everything, return success
		bool parse(const string &content);

		// advanced: iterative parsing
		bool begin();
		bool step(const string &content, bool last = false);
		bool end();
		bool more() const;
		const Result &result() const;

		// optional grammar determinism check after a successful parse
		bool nextMatch();

	protected:
		bool last();

	private:
		Rule theStartRule;
		Rule theSkipRule;

		Buffer theBuffer;
		Result theResult;
};

} // namespace

#endif

