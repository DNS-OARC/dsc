 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_NET_ADDR__H
#define TMF_BASE__XSTD_NET_ADDR__H

#include "xstd/xstd.h"

#include <string>
#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h> /* for TCP_NODELAY */
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if defined(HAVE_WINSOCK2_H) && !defined(_WINSOCK2API_)
#include <winsock2.h>
#endif

// this is needed on Solaris; others?
#ifndef INADDR_NONE
#	ifndef _IN_ADDR_T
#		define _IN_ADDR_T
		typedef unsigned int in_addr_t;
#	endif
#	define INADDR_NONE ((in_addr_t)-1)
#endif

#if !defined(HAVE_INET_MAKEADDR)
	extern in_addr inet_makeaddr(unsigned long net, unsigned long lna);
	extern unsigned long inet_lnaof(in_addr in);
	extern unsigned long inet_netof(in_addr in);
#endif

inline bool operator ==(in_addr a, in_addr b) { return a.s_addr == b.s_addr; }
inline bool operator !=(in_addr a, in_addr b) { return a.s_addr != b.s_addr; }

// inet_addr wrapper which also maintains optional port and FQDN
class NetAddr {
	public:
		static NetAddr MakeAddr(unsigned long net, unsigned long lna, int port = -1);
		static in_addr NetMask(int subnet);

	public:
		NetAddr(): thePort(-1) { theAddrN.s_addr = INADDR_NONE; }
		NetAddr(const NetAddr &na): theAddrA(na.theAddrA), theAddrN(na.theAddrN), thePort(na.thePort) {}
		NetAddr(const string &addr, int aPort): theAddrA(addr), thePort(aPort) { theAddrN.s_addr = INADDR_NONE; }
		NetAddr(const in_addr &addr, int aPort): theAddrN(addr), thePort(aPort) {}
		NetAddr(const sockaddr_in &a): theAddrN(a.sin_addr), thePort(a.sin_port) {}

		bool known() const { return theAddrA.size() || theAddrN.s_addr != INADDR_NONE || thePort >= 0; }
		operator void*() const { return known() ? (void*)-1 : 0; }
		bool operator < (const NetAddr &addr) const;
		bool operator == (const NetAddr &addr) const;
		bool operator != (const NetAddr &addr) const { return !(*this == addr); }
		bool sameButPort(const NetAddr &addr) const;

		const string &addrA() const { if (!theAddrA.size()) syncA(); return theAddrA; }
		const in_addr &addrN() const { if (theAddrN.s_addr == INADDR_NONE) syncN(); return theAddrN; }
		unsigned long lna() const;
		unsigned long net() const;
		int octet(int idx) const; // a.b.c.d = 1.2.3.4
		int port() const { return thePort; }

		void addr(const string &addr) { theAddrA = addr; theAddrN.s_addr = INADDR_NONE; }
		void addr(in_addr addr) { theAddrN = addr; theAddrA = string(); }
		void addr(const string &a, in_addr n) { theAddrA = a; theAddrN = n; }
		void port(int aPort) { thePort = aPort; }

		ostream &print(ostream &os) const;

		bool isDomainName() const;

	protected:
		void syncA() const; // theAddrA <- theAddrN
		void syncN() const; // theAddrN <- theAddrA

	protected:
		mutable string theAddrA;  // FQDN or IP; set if not empty
		mutable in_addr theAddrN; // IP if available; set if >= 0;
		int thePort;              // set if >= 0;
};

inline ostream &operator <<(ostream &os, const NetAddr &addr) { return addr.print(os); }

#endif
