 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

// select(2) system call
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "xstd/Assert.h"
#include "xstd/Math.h"
#include "xstd/Clock.h"
#include "xstd/Select.h"


Select TheSelector;


/* SelectUser */

SelectUser::~SelectUser() {
}

// a user may not want to implement read or write
// the default implementation will barf in case the
// corresponding method is actually called
void SelectUser::noteReadReady(int) {
	Assert(false);
}

void SelectUser::noteWriteReady(int) {
	Assert(false);
}

void SelectUser::noteTimeout(int, Time) {
	Assert(false);
}


/* SelectReserv */

SelectReserv::SelectReserv(): theFD(-1), theDir(dirNone), isReady(false) {
}

SelectReserv::SelectReserv(int aFD, IoDir aDir): theFD(aFD), theDir(aDir), isReady(false) {
}


/* SelectUserReg */

// note: any activity on the Reg record should reset theStart member
// to upgrade timeout expiration

void SelectUserReg::reset() {
	theUser = 0;
	theStart = theTimeout = Time();
	theResCount = 0;
}

void SelectUserReg::set(SelectUser *aUser) {
	Assert(aUser);
	if (theUser) {
		Assert(theUser == aUser);
		Assert(theResCount == 1);
	} else {
		theUser = aUser;
		theTimeout = Time();
	}
	theStart = TheClock;
	theResCount++;
}

void SelectUserReg::clear() {
	Assert(theUser);
	Assert(theResCount > 0);
	if (--theResCount == 0)
		reset();
	else
		theStart = TheClock;
}

void SelectUserReg::timeout(Time aTout) {
	theTimeout = aTout;
}

Time SelectUserReg::waitTime() const {
	return TheClock - theStart;
}

bool SelectUserReg::timedout() const {
	return theTimeout >= 0 && theTimeout <= waitTime();
}

bool SelectUserReg::needsCheck() const {
	return theUser;
}

void SelectUserReg::noteCheck(bool didIO) {
	if (theUser && didIO)
		theStart = TheClock;
}


/* Select */

Select::Select(): theMaxFD(-1), theResCount(0) {
}

Select::~Select() {
}

void Select::configure(int fdLimit) {
	theSets[dirRead].fdLimit(fdLimit);
	theSets[dirWrite].fdLimit(fdLimit);
}

void Select::checkTimeouts() {
	for (int fd = 0; fd <= theMaxFD; ++fd) {
		SelectUserReg &reg = theRegs[fd];
		if (reg.timedout()) {
			if (User *u = reg.user())
				u->noteTimeout(fd, reg.waitTime());
		}
	}
}

int Select::scan(Time *timeout) {

	if (timeout && *timeout < 0) // no time to sweep
		return 0;

	// silent fix:
	// unfortunately, some selects(2) have magic limits on timeouts
	if (timeout && timeout->tv_sec > 100000000)
		timeout->tv_sec = 100000000;

	//cerr << here << "reserv: " << theResCount << endl;
	const int hotCount = sweep(timeout);
	//cerr << here << "hot files: " << hotCount << endl;

	for (int idx = 0; idx < hotCount; ++idx) {
		int fd;
		bool active = false;
		//cerr << here << "idx: " << idx << " ready: " << readyUser(idx, dirRead, fd) << " & " << readyUser(idx, dirWrite, fd) << endl;
		if (User *u = readyUser(idx, dirRead, fd)) {
			theRegs[fd].noteCheck(true);
			u->noteReadReady(fd);
			active = true;
		}
		if (User *u = readyUser(idx, dirWrite, fd)) {
			theRegs[fd].noteCheck(true);
			u->noteWriteReady(fd);
			active = true;
		}
	}

	checkTimeouts();

	return hotCount;
}

SelectReserv Select::setFD(int fd, IoDir dir, SelectUser *u) {
	theSets[dir].setFD(fd, u);
	if (fd > theMaxFD)
		theMaxFD = fd;

	theResCount++;
	theRegs[fd].set(u);
	return SelectReserv(fd, dir);
}

void Select::setTimeout(int fd, Time tout) {
	Assert(fd >= 0 && fd <= theMaxFD);
	theRegs[fd].timeout(tout);
}

void Select::clearTimeout(int fd) {
	theRegs[fd].timeout(Time());
}

void Select::clearFD(int fd, IoDir dir) {
	theSets[dir].clearFD(fd);
	if (fd == theMaxFD)
		theMaxFD = Max(theSets[dirRead].maxFD(), theSets[dirWrite].maxFD());

	theRegs[fd].clear();
	theResCount--;
}

SelectUser *Select::readyUser(int idx, IoDir dir, int &fd) {
	fd = idx;
	return theSets[dir].isReady(fd) ? theRegs[fd].user() : 0;
}

int Select::sweep(Time *timeout) {
	// have to check all fds, cannot choose like poll() does
	const int hotCount = theMaxFD+1;

	fd_set *rdSet = theSets[dirRead].makeReadySet();
	fd_set *wrSet = theSets[dirWrite].makeReadySet();
	fd_set *excSet = 0;
#	if defined(WIN32)
		// MS Windows reports connect(2) failures via exception set
		// collect exceptions and later "backport" to wrSet to fake
		// Unix-like behavior
		if (wrSet) {
			static fd_set es;
			es = *wrSet;
			excSet = &es;
		} else
		if (!rdSet) {
			// Windows select(2) fails if no FDs are checked for
			// we must emulate sleep instead of calling select(2)
			if (timeout)
				sleep(timeout->sec());
			return 0;
		}
#	endif

	const int res = ::select(hotCount, rdSet, wrSet, excSet, timeout);

#	if defined(WIN32)
		if (res > 0 && excSet && wrSet) {
			for (int fd = 0; fd < hotCount; ++fd) {
				if (FD_ISSET(fd, excSet))
					FD_SET(fd, wrSet);
			}
		}
#	endif

	return res <= 0 ? res : hotCount;
}


/* FD_Set */

FD_Set::FD_Set(): theMaxFD(-1), theResCount(0) {
	FD_ZERO(&theSet);
	FD_ZERO(&theReadySet);
}

FD_Set::~FD_Set() {
}

void FD_Set::fdLimit(int) {
}

void FD_Set::setFD(int fd, SelectUser *u) {
	Assert(fd >= 0 && u);

	FD_SET(fd, &theSet);
	if (fd > theMaxFD)
		theMaxFD = fd;
	theResCount++;
}

void FD_Set::clearFD(int fd) {
	Assert(fd >= 0);
	Assert(fd <= theMaxFD);

	FD_CLR(fd, &theSet);
	FD_CLR(fd, &theReadySet); // poll() cannot do that

	if (fd == theMaxFD) {
		while (theMaxFD >= 0 && !FD_ISSET(theMaxFD, &theSet)) {
			theMaxFD--;
		}
	}

	theResCount--;
}

