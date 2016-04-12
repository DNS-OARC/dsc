// this file is used to check that actions.html examples still compile

#include <Hapy/Rule.h>
#include <Hapy/Rules.h>
#include <Hapy/Actions.h>
#include <Hapy/IoStream.h>

using namespace Hapy;
using namespace std;

static
void TurnDebuggingOn(Action::Params *) {
	Debug(true);
}

void ex1() {
	Rule particular;
	particular = anychar_r;
	particular.action(&TurnDebuggingOn);
}

void ex1i() {
	Rule particular;
	particular = anychar_r[&TurnDebuggingOn];
}


#include <functional>

static
void ExplainDebuggingOn(Action::Params *, const char *reason) {
	clog << "turning debugging on because " << reason << endl;
	Debug(true);
}

void ex3() {
	Rule particular1, particular2;
	Rule combo = particular1 | particular2;
	particular1.action(
		bind2nd(ptr_fun(&ExplainDebuggingOn), "tricky rule matched"));
	particular2.action(
		bind2nd(ptr_fun(&ExplainDebuggingOn), "coredump is near"));
}

void ex3i() {
	Rule particular1, particular2;
	Rule combo =
		particular1[
			bind2nd(ptr_fun(&ExplainDebuggingOn), "tricky rule matched")] |
		particular2[
			bind2nd(ptr_fun(&ExplainDebuggingOn), "coredump is near")];
}



#include <list>

typedef std::list<Pree> Matches;

static
void AddMatch(Action::Params *p, Matches *matches) {
	matches->push_back(p->pree);
}

void ex4() {
	Matches matches;
	Rule particular;
	particular = anychar_r;
	particular.action(
		bind2nd(ptr_fun(&AddMatch), &matches));
									   
}

void ex4i() {
	Matches matches;
	Rule particular;
	particular = anychar_r[
		bind2nd(ptr_fun(&AddMatch), &matches)];
}

using namespace Hapy;

class Interpreter {
	public:
		void handleAssignment(Action::Params *) {}
		void handleOption(Action::Params *) {}
};


void ex5() {
	Interpreter interpreter;
	Rule grammar, statement, assignment, option, name, value;

	grammar = *(statement >> ";");
	statement = assignment | option;
	assignment = name >> "=" >> value;
	option = name >> *value;

	assignment.action(mem_action(&interpreter, &Interpreter::handleAssignment));
	option.action(mem_action(&interpreter, &Interpreter::handleOption));
}

void ex5i() {
	Interpreter interpreter;
	Rule grammar, statement, assignment, option, name, value;

	grammar = *(statement >> ";");
	statement = assignment | option;
	assignment = (name >> "=" >> value)[
		mem_action(&interpreter, &Interpreter::handleAssignment)];
	option = (name >> *value)[
		mem_action(&interpreter, &Interpreter::handleOption)];
}




static
void BeforeAction(Action::Params *) {
	// ...
}

static
void AfterAction(Action::Params *) {
	// ...
}

void ex6() {
	Rule before = empty_r;  // zero width rule
	before.action(&BeforeAction); 

	Rule realCore;  // the actual rule we are interested in
	realCore = *anychar_r;
	realCore.action(&AfterAction);

	Rule newCore = before >> realCore; 
	// use "newCore" instead of the "realCore" rule
}

void ex6i() {
	Rule before = empty_r[&BeforeAction];

	Rule realCore;  // the actual rule we are interested in
	realCore = (*anychar_r)[&AfterAction];

	Rule newCore = before >> realCore; 
	// use "newCore" instead of the "realCore" rule
}

int main() {
	ex1();
	ex1i();

	ex3();
	ex3i();

	ex4();
	ex4i();

	ex5();
	ex5i();

	ex6();
	ex6i();

	return 0;
}
