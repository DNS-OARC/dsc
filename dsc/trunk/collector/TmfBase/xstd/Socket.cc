 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <iostream>
#include <strstream>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h> /* for TCP_NODELAY */
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "xstd/Assert.h"
#include "xstd/Socket.h"


int  Socket::TheMaxLevel = -1;
int  Socket::TheLevel = 0;


void Socket::Configure() {
#ifdef HAVE_WSASTARTUP
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	Must(WSAStartup(wVersionRequested, &wsaData) == 0);
	/* Confirm that the WinSock DLL supports 2.2.*/
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		WSACleanup( );
		Must(false);
	}
#endif
}

void Socket::Clean() {
#ifdef HAVE_WSACLEANUP
	WSACleanup();
#endif
}

void Socket::TooManyFiles() {
#ifdef WSAEMFILE
	Error::Last(WSAEMFILE);
#else
	Error::Last(EMFILE);
#endif
}

// called when theFd is set to a new value
bool Socket::noteNewFD() {
	if (theFD < 0)
		return false;

	TheLevel++;

	// most MaxLevel users assume that MaxLevel means max FD!
	if (TheMaxLevel >= 0 && theFD >= TheMaxLevel) {
		close();
		TooManyFiles();
		return false;
	}

	return true;
}

bool Socket::create() {
	return create(PF_INET, SOCK_STREAM, 0);
}

bool Socket::create(int domain, int type, int protocol) {

	if (TheMaxLevel >= 0 && TheLevel >= TheMaxLevel) {
		TooManyFiles();
		return false;
	}
	
	theFD = ::socket(domain, type, protocol);
	return noteNewFD();
}

bool Socket::bind(const NetAddr &addr) {
	static sockaddr_in S;

	S.sin_family = AF_INET;
	S.sin_port = htons(addr.port() >= 0 ? addr.port() : 0);
	if (addr.addrA().size())
		S.sin_addr = addr.addrN();
	else
		S.sin_addr.s_addr = htonl(INADDR_ANY);

	return ::bind(theFD, (sockaddr *) &S, sizeof(S)) == 0;
}

bool Socket::connect(const NetAddr &addr) {
	static sockaddr_in s;

	s.sin_family = AF_INET;
	s.sin_addr = addr.addrN();
	s.sin_port = htons(addr.port());

	if (::connect(theFD, (sockaddr *) &s, sizeof(s)) < 0) {
		const Error err = Error::Last();
		return err == EINPROGRESS || err == EWOULDBLOCK;
	}
	return true;
}

bool Socket::listen() {
	return ::listen(theFD, FD_SETSIZE) == 0;
}

Socket Socket::accept(sockaddr *addr_p, socklen_t *addr_len_p) {

	if (TheMaxLevel >= 0 && TheLevel >= TheMaxLevel) {
		TooManyFiles();
		return -1;
	}

	sockaddr addr;
	socklen_t addr_len = sizeof(addr);
	if (!addr_p) {
		addr_p = &addr;
		addr_len_p = &addr_len;
	} else
		Assert(addr_len_p);

	const int fd = ::accept(theFD, addr_p, addr_len_p);

	Socket s(fd);
	s.noteNewFD();
	return s;
}

Socket Socket::accept(NetAddr &addr) {
	sockaddr_in addr_in;
	socklen_t addr_len = sizeof(addr_in);
	const Socket s = accept((sockaddr*)&addr_in, &addr_len);
	addr.addr(addr_in.sin_addr);
	addr.port(ntohs(addr_in.sin_port));
	return s;
}

Size Socket::read(void *buf, Size sz) {
	return sz > 0 ? sysRead(buf, sz) : (Size)0;
}

Size Socket::write(const void *buf, Size sz) {
	return sz > 0 ? sysWrite(buf, sz) : sz;
}

Size Socket::recvFrom(void *buf, Size sz, NetAddr &addr, int flags) {
	if (sz <= 0)
		return 0;

	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr = addr.addrN();
	sa.sin_port = htons(addr.port());

	socklen_t addr_len = sizeof(sa);

	// must cast to char* on Solaris
	const Size readSz =
		(Size)::recvfrom(theFD, (char*)buf, sz, flags, (sockaddr *) &sa, &addr_len);

	if (readSz >= 0) {
		addr.addr(sa.sin_addr);
		addr.port(ntohs(sa.sin_port));
	}
	return readSz;
}

Size Socket::sendTo(const void *buf, Size sz, const NetAddr &addr, int flags) {
	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr = addr.addrN();
	sa.sin_port = htons(addr.port());

	// must cast to char* on Solaris
	return sz > 0 ?
		(Size)::sendto(theFD, (const char *)buf, sz, flags, (const sockaddr *) &sa, sizeof(sa)) :
		(Size)0;
}

bool Socket::close() {
	if (theFD >= 0) {
		const bool res = sysClose();
		theFD = -1;
		TheLevel--;
		return res;
	}
	Error::Last(EBADF);
	return false;
}

bool Socket::sendBuf(Size sz) {
#ifdef SO_SNDBUF
	return setOpt(SO_SNDBUF, (const void*)&sz, sizeof(sz));
#else
	return unsupOpt(&sz);
#endif
}

bool Socket::recvBuf(Size sz) {
#ifdef SO_RCVBUF
	return setOpt(SO_RCVBUF, (const void*)&sz, sizeof(sz));
#else
	return unsupOpt(&sz);
#endif
}

bool Socket::nagle(int nagle) {
	if (nagle && !tcpDelay(nagle > 0))
		return false;
	return true;
}

int Socket::flags() const {
#if defined(HAVE_FCTL) && defined(F_GETFL)
	return fcntl(theFD, F_GETFL);
#else
	Error::Last(EINVAL);
	return 0;
#endif
}

/* local port */
int Socket::lport() const {
	sockaddr_in S;
	socklen_t len = sizeof(S);

	if (getsockname(theFD, (sockaddr*)&S, &len) == 0)
		return ntohs(S.sin_port);
	else
		return -1;
}

/* remote (peer) port */
int Socket::rport() const {
	sockaddr_in S;
	socklen_t len = sizeof(S);

	if (getpeername(theFD, (sockaddr*)&S, &len) == 0)
		return ntohs(S.sin_port);
	else
		return -1;
}

Time Socket::rcvTimeout() const {
#ifdef SO_RCVTIMEO
	Time t;
	socklen_t len = sizeof(t);
	if (getOpt(SO_RCVTIMEO, (void*)&t, &len))
		return t;
#endif
	return Time();
}

Size Socket::sendBuf() const {
#ifdef SO_SNDBUF
	Size sz;
	socklen_t len = sizeof(sz);
	if (getOpt(SO_SNDBUF, (void*)&sz, &len))
		return sz;
#endif
	return Size();
}

Size Socket::recvBuf() const {
#ifdef SO_RCVBUF
	Size sz;
	socklen_t len = sizeof(sz);
	if (getOpt(SO_RCVBUF, (void*)&sz, &len))
		return sz;
#endif
	return Size();
}

/*
 * Note: Solaris 2.4's socket emulation doesn't allow you
 * to determine the error from a failed non-blocking
 * connect and just returns EPIPE. -- fenner@parc.xerox.com
 */
Error Socket::error() const {
	int en;
	socklen_t len = sizeof(en);

	if (getOpt(SO_ERROR, (void*)&en, &len))
		return Error::Last(en);
	else
		return Error::Last();
}

bool Socket::peerAddr(sockaddr *S, socklen_t &len) const {
	return getpeername(theFD, S, &len) == 0;
}

NetAddr Socket::localAddr() const {
	sockaddr_in s;
	socklen_t len = sizeof(s);
	if (getsockname(theFD, (sockaddr*)&s, &len) == 0)
		return NetAddr(s.sin_addr, ntohs(s.sin_port));
	else
		return NetAddr();
}

NetAddr Socket::peerAddr() const {
	sockaddr_in s;
	socklen_t len = sizeof(s);
	if (peerAddr((sockaddr*) &s, len))
		return NetAddr(s.sin_addr, ntohs(s.sin_port));
	else
		return NetAddr();
}

bool Socket::reuseAddr(bool set) {
#ifdef SO_REUSEADDR
	return setOpt(SO_REUSEADDR, set);
#else
	return unsupOpt(&set);
#endif
}

bool Socket::linger(Time delay) {
#ifdef SO_LINGER
	static struct linger l = // are casts OS-specific?
		{ (bool)(delay.sec() > 0), (unsigned short) delay.sec() };
	return setOpt(SO_LINGER, &l, sizeof(l));
#else
	return unsupOpt(&delay);
#endif
}

bool Socket::blocking(bool block) {
#ifdef FIONBIO
	unsigned long param = !block;
	return ioctl(theFD, FIONBIO, &param) == 0;
#else
	const int oldFlags = flags();
	int newFlags = oldFlags;

	if (block)
		newFlags &= ~O_NONBLOCK;
	else
		newFlags |=  O_NONBLOCK;

	if (newFlags == oldFlags)
		return true;

	return fcntl(theFD, F_SETFL, newFlags) != -1;
#endif
}

bool Socket::reusePort(bool set) {
#ifdef SO_REUSEPORT
	return setOpt(SO_REUSEPORT, set);
#else
	return unsupOpt(&set);
#endif
}

bool Socket::tcpDelay(bool set) {
#ifdef TCP_NODELAY
	int optVal = (int)!set;
	// must cast to char* on Solaris
	return ::setsockopt(theFD, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, sizeof(optVal)) == 0;
#else
	return unsupOpt(&set);
#endif
}

bool Socket::isBlocking() const {
#ifdef FIONBIO
	int nb = 0;
	socklen_t len = sizeof(nb);
	return getOpt(FIONBIO, &nb, &len) ? !nb : true;
#else
	return flags() & O_NONBLOCK == 0;
#endif
}


bool Socket::setOpt(int optName, bool set) {
	int optVal = (int)set;
	return setOpt(optName, &optVal, sizeof(optVal));
}

bool Socket::setOpt(int optName, const void *optVal, socklen_t optLen) {
	return setOpt(SOL_SOCKET, optName, (const char*)optVal, optLen);
}

bool Socket::getOpt(int optName, void *optVal, socklen_t *optLen) const {
	return getOpt(SOL_SOCKET, optName, (char*)optVal, optLen);
}

bool Socket::setOpt(int level, int optName, const void *optVal, socklen_t optLen) {
	// must cast to char* on Solaris
	return ::setsockopt(theFD, level, optName, (const char*)optVal, optLen) == 0;
}

bool Socket::getOpt(int level, int optName, void *optVal, socklen_t *optLen) const {
	// must cast to char* on Solaris
	return ::getsockopt(theFD, level, optName, (char*)optVal, optLen) == 0;
}

// W2K do not have read/write/close for sockets, only files
Size Socket::sysRead(void *buf, Size sz) {
#	ifdef HAVE_CLOSESOCKET
		return (Size)::recv(theFD, (char*)buf, sz, 0);
#	else
		return (Size)::read(theFD, buf, sz);
#	endif
}

Size Socket::sysWrite(const void *buf, Size sz) {
#	ifdef HAVE_CLOSESOCKET
		return (Size)::send(theFD, (const char*)buf, sz, 0);
#	else
		return (Size)::write(theFD, buf, sz);
#	endif
}

bool Socket::sysClose() {
#	ifdef HAVE_CLOSESOCKET
		return !::closesocket(theFD);
#	else
		return !::close(theFD);
#	endif
}


// the argument is used by the caller to prevent "arg not used" warnings
bool Socket::unsupOpt(void *) const {
	Error::Last(ENOPROTOOPT);
	return false;
}


