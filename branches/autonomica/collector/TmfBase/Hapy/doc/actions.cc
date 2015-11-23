// this file is used to check that actions.html examples still compile

#include <Hapy/Action.h>
#include <Hapy/Rule.h>
#include <Hapy/Rules.h>
#include <Hapy/IoStream.h>

using namespace Hapy;
using namespace std;

static
void TurnDebuggingOn1(Action::Params *) {
	Rule::Debug(true);
}

void ex1() {
	Rule particular;
	particular = anychar_r;
	particular.ptr_action(&TurnDebuggingOn1);
}




void ex2() {
	Rule particular;
	particular = anychar_r;
	particular.action(ptr_action(&TurnDebuggingOn1));
}


#include <functional>

static
void TurnDebuggingOn2(Action::Params *, const char *reason) {
	clog << "turning debugging on because " << reason << endl;
	Rule::Debug(true);
}

void ex3() {
	Rule particular1, particular2;
	particular1.action(ptr_action(
		bind2nd(ptr_fun(&TurnDebuggingOn2), "tricky rule matched")));
	particular2.action(ptr_action(
		bind2nd(ptr_fun(&TurnDebuggingOn2), "coredump is near")));
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
	particular.action(ptr_action(
		bind2nd(ptr_fun(&AddMatch), &matches)));
									   
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
	before.ptr_action(&BeforeAction); 

	Rule realCore;  // the actual rule we are interested in
	realCore = *anychar_r;
	realCore.ptr_action(&AfterAction);

	Rule newCore = before >> realCore; 
	// use "newCore" instead of the "realCore" rule
}

int main() {
	ex1();
	ex2();
	ex3();
	ex4();
	ex5();
	ex6();
	return 0;
}
