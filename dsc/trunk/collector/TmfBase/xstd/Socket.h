 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_SOCKET__H
#define TMF_BASE__XSTD_SOCKET__H

#include <sys/types.h>
#include <sys/socket.h>

#include "xstd/Time.h"
#include "xstd/Size.h"
#include "xstd/Error.h"
#include "xstd/NetAddr.h"


// socket wrapper
class Socket {
	public:
		static void Configure();
		static void Clean();
		static int Level() { return TheLevel; }
		static void TooManyFiles();
		static int TheMaxLevel; // will not allow Level to go higher

	public:
		Socket(): theFD(-1) {}
		Socket(int aFD): theFD(aFD) {}
		~Socket() {}

		bool create();
		bool create(int domain, int type, int protocol);
		bool connect(const NetAddr &addr);
		bool bind(const NetAddr &addr);
		bool listen();
		Socket accept(struct sockaddr *addr, socklen_t *addr_len);
		Socket accept(NetAddr &addr);
		Size read(void *buf, Size sz);
		Size write(const void *buf, Size sz);
		Size recvFrom(void *buf, Size sz, NetAddr &addr, int flags = 0);
		Size sendTo(const void *buf, Size sz, const NetAddr &addr, int flags = 0);
		bool close();

		bool reuseAddr(bool set);
		bool reusePort(bool set);
		bool blocking(bool set);
		bool tcpDelay(bool set);
		bool sendBuf(Size size);
		bool recvBuf(Size size);

		bool linger(Time tout);
		bool nagle(int nagle);

		int fd() const { return theFD; }
		bool isOpen() const { return theFD >= 0; }
		bool isBlocking() const;
		int flags() const;
		int lport() const;
		int rport() const;
		Time rcvTimeout() const;
		Size sendBuf() const;
		Size recvBuf() const;
		Error error() const;
		NetAddr localAddr() const;
		bool peerAddr(struct sockaddr *addr, socklen_t &len) const;
		NetAddr peerAddr() const;

		operator void*() const { return theFD >= 0 ? (void*)-1 : (void*)0; }

		// always treat two these methods as protected!
		bool setOpt(int level, int optName, const void *optVal, socklen_t optLen);
		bool getOpt(int level, int optName, void *optVal, socklen_t *optLen) const;

	protected:
		bool noteNewFD();

		bool setOpt(int optName, bool set);
		bool setOpt(int optName, const void *optVal, socklen_t optLen);
		bool getOpt(int optName, void *optVal, socklen_t *optLen) const;

		Size sysRead(void *buf, Size sz);
		Size sysWrite(const void *buf, Size sz);
		bool sysClose();

		bool unsupOpt(void *dummy = 0) const;

	protected:
		int theFD;

		static int TheLevel; // number of open sockets
};

#endif
