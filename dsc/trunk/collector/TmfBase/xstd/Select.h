 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_SELECT__H
#define TMF_BASE__XSTD_SELECT__H

#include <map>
#include "xstd/Time.h"

enum IoDir { dirNone = -1, dirRead = 0, dirWrite = 1 };

class SelectUser {
	public:
		virtual ~SelectUser();

		// default implementation will just abort
		virtual void noteReadReady(int fd);
		virtual void noteWriteReady(int fd);
		virtual void noteTimeout(int fd, Time tout);
};

// select reservation receipt
class SelectReserv {
	public:
		SelectReserv();
		SelectReserv(int aFD, IoDir aDir);

		operator bool() const { return reserved(); }
		bool reserved() const { return theFD >= 0; }
		bool ready() const { return isReady; }
		int fd() const { return theFD; }
		IoDir dir() const { return theDir; }

		void ready(bool be) { isReady = be; }
		void clear() { theFD = -1; }

	protected:
		int theFD;
		IoDir theDir;
		bool isReady;
};

// a wrapper arround fd_set
class FD_Set {
	typedef SelectUser User;
	public:
		FD_Set();
		~FD_Set();

		void fdLimit(int limit);

		int maxFD() const { return theMaxFD; }
		int resCount() const { return theResCount; }
		bool active() const { return theMaxFD >= 0; }
		bool readyActive() const { return theReadyMaxFD >= 0; }
		fd_set *makeReadySet() { theReadySet = theSet; theReadyMaxFD = theMaxFD; return readyActive() ? &theReadySet : 0; }

		void setFD(int fd, User *u);
		void clearFD(int fd);

		bool isSet(int fd) const { return FD_ISSET(fd, &theSet); }
		bool isReady(int fd) const { return FD_ISSET(fd, &theReadySet); }

	protected:
		fd_set theSet;
		fd_set theReadySet;

		int theMaxFD;
		int theReadyMaxFD;
		int theResCount;
};


// an internal registration record for a user to monitor I/O activity
class SelectUserReg {
	public:
		SelectUserReg() { reset(); }

		void reset();

		void set(SelectUser *aUser); // updates start time and res counter
		void clear(); // decrements reservation counter
		void timeout(Time aTout);

		SelectUser *user() { return theUser; }
		Time waitTime() const;

		bool timedout() const;
		bool needsCheck() const;

		void noteCheck(bool didIO);

	protected:
		SelectUser *theUser;
		Time theTimeout;         // how long a user is willing to wait for an io
		Time theStart;           // registration or last io
		int theResCount;         // how many reservations (e.g., read + write)
};

// select(2) system call wrapper
class Select {
	public:
		typedef SelectUser User;
		friend class SelectUserReg;

	public:
		Select();
		~Select();

		void configure(int fdLimit);

		bool idle() const { return theResCount <= 0; }

		SelectReserv setFD(int fd, IoDir dir, User *p);
		void clearFD(int fd, IoDir dir);
		void clearRes(SelectReserv &res) { clearFD(res.fd(), res.dir()); res.clear(); }

		void setTimeout(int fd, Time tout);
		void clearTimeout(int fd);

		// returns a positive number if some files where ready; -1 on error
		// calls applicable User methods for ready files
		// null timeout means wait until error or at least one file is ready
		int scan(Time *timeout = 0);

	protected:
		// returns "hot count" (see scan code)
		int sweep(Time *timeout = 0);

		// must set fd if the indexed user is ready
		User *readyUser(int idx, IoDir dir, int &fd);

		void checkTimeouts();

	protected:
		map<int, SelectUserReg> theRegs; // user registrations
		FD_Set theSets[2]; // read/write
		int theMaxFD;
		int theResCount;
};

extern Select TheSelector;

#endif
