/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_RULE_BASE__H
#define HAPY_RULE_BASE__H

#include <Hapy/Top.h>
#include <Hapy/Result.h>
#include <Hapy/String.h>
#include <Hapy/IosFwd.h>
#include <Hapy/RuleId.h>
#include <Hapy/RuleCompFlags.h>
#include <Hapy/First.h>
#include <Hapy/SizeCalc.h>
#include <Hapy/Action.h>
#include <Hapy/DeepPrint.h>
#include <Hapy/RulePtr.h>
#include <list>

namespace Hapy {

class Buffer;
class Pree;
class Algorithm;
class Rule;

// grammar rule implementation: parsing alg + ID + flags and logic
// see Rule for user-level grammar rule interface
class RuleBase {
	public:
		typedef Result::StatusCode StatusCode;
		typedef RuleCompFlags CFlags;		
		typedef std::list<RulePtr> CRules;

		// announce parsing rule rejection due to the given reason
		static void DebugReject(const RuleBase *rule, const char *reason);

	public:
		RuleBase();

		void id(const RuleId &anId);

		const RuleId &id() const { return theId; }

		bool build(const CFlags &flags); // called by parser
		bool compile(const CFlags &flags); // called by algorithms
		bool compiling() const { return isCompiling; }

		void collectRules(CRules &rules);
		int subrules() const;

		SizeCalcLocal::size_type calcMinSize(SizeCalcPass &pass);
		SizeCalcLocal::size_type minSize() const;

		void calcFullFirst();
		bool calcPartialFirst(First &first, Pree &pree);

		void committed(bool be);

		bool terminal() const;
		bool trimming() const;
		void trim(const RulePtr &aTrimmer);
		void verbatim(bool be);
		void leaf(bool be);
		void action(const Action &anAction);

		StatusCode firstMatch(Buffer &buf, Pree &pree) const;
		StatusCode nextMatch(Buffer &buf, Pree &pree) const;
		StatusCode resume(Buffer &buf, Pree &pree) const;
		void cancel(Buffer &buf, Pree &pree) const;

		ostream &print(ostream &os) const;
		void deepPrint(ostream &os, DeepPrinted &printed) const;

		bool hasAlg() const { return theAlg != 0; }
		const Algorithm &alg() const;
		void alg(Algorithm *anAlg);
		void updateAlg(const RuleBase &src);

		void implicit(bool be);

		bool temporary() const;

	protected:
		bool shouldTrim(CFlags &flags) const;
		bool compileTrim(const CFlags &flags);

		bool calcMinSize(CRules &rules);

		bool mayMatch(Buffer &buf) const;

		typedef StatusCode (Algorithm::*AlgMethod)(Buffer &buf, Pree &pree) const;
		StatusCode call(Buffer &buf, Pree &pree, AlgMethod m, const char *mLabel) const;
		StatusCode applyAction(Buffer &buf, Pree &pree) const;
		
		static ostream &DebugPfx(int callId);
		void debugBuffer(const Buffer &buf) const;
		void debugTry(const Buffer &buf, const Pree &pree, const char *mLabel, int callId) const;
		void debugResult(const Buffer &buf, const Pree &pree, const char *mLabel, int callId, StatusCode result) const;

	protected:
		Algorithm *theAlg;
		Action theAction;
		RuleId theId;

		RulePtr theTrimmer;

		First theFirst;
		enum { emsUnknown, emsComputing, emsYes, emsNo } theEmptyMatchState;
		enum { fssUnknown, fssComputing, fssKnown, fssError } theFirstSetState;

		SizeCalcLocal theSizeCalc;
		int theSubrules; // number of subrules

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
