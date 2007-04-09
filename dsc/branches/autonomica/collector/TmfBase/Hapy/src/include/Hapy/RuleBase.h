/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULE_BASE__H
#define HAPY_RULE_BASE__H

#include <Hapy/config.h>
#include <Hapy/Result.h>
#include <Hapy/HapyString.h>
#include <Hapy/IosFwd.h>
#include <Hapy/RuleId.h>
#include <Hapy/RulePtr.h>

namespace Hapy {

class Buffer;
class Pree;
class Algorithm;
class Action;
class Rule;

// grammar rule implementation: parsing alg + ID + flags and logic
// see Rule for user-level grammar rule interface
class RuleBase {
	public:
		typedef Result::StatusCode StatusCode;
		typedef enum { dbdNone, dbdUser, dbdAll } DebugDetail;
		static DebugDetail TheDebugDetail; // debugging detail level

	public:
		RuleBase();

		void id(const RuleId &anId);

		const RuleId &id() const { return theId; }

		bool compile(RulePtr itrimmer);

		void committed(bool be);

		bool terminal() const;
		bool trimming() const;
		void trim(const RulePtr &aTrimmer);
		void verbatim(bool be);
		void leaf(bool be);
		void action(const Action *anAction);

		StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		StatusCode resume(Buffer &buf, Pree &pree) const;
		void cancel(Buffer &buf, Pree &pree) const;

		ostream &print(ostream &os) const;

		bool hasAlg() const { return theAlg != 0; }
		const Algorithm &alg() const;
		void alg(Algorithm *anAlg);
		void updateAlg(const RuleBase &src);
		void reachEnd(bool doIt);

		bool temporary() const;

	protected:
		static bool Debug(DebugDetail codeLevel);

		void implicitTrim(const RulePtr &trimmer);
		bool shouldTrim() const;
		bool compileTrim();

		typedef StatusCode (Algorithm::*AlgMethod)(Buffer &buf, Pree &pree) const;
		StatusCode call(Buffer &buf, Pree &pree, AlgMethod m, const char *mLabel) const;
		StatusCode applyAction(Buffer &buf, Pree &pree) const;
		
		ostream &debugPfx(int callId) const;
		void debugBuffer(const Buffer &buf) const;
		void debugTry(const Buffer &buf, const Pree &pree, const char *mLabel, int callId) const;
		void debugResult(const Buffer &buf, const Pree &pree, const char *mLabel, int callId, StatusCode result) const;

	protected:
		Algorithm *theAlg;
		const Action *theAction;
		RuleId theId;

		RulePtr theTrimmer;

		enum { cmDefault, cmDont, cmCommit } theCommitMode;
		enum { tmDefault, tmVerbatim, tmImplicit, tmExplicit } theTrimMode;
		bool isImplicit;
		bool isLeaf;
		bool mustReachEnd;

		bool isCompiling;

	private:
		static int TheLastCallId;
		static int TheCallLevel;
};

} // namespace

#endif
