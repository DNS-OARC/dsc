 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <iostream>

#include "xstd/Assert.h"
#include "xstd/NetAddr.h"


NetAddr NetAddr::MakeAddr(unsigned long net, unsigned long lna, int port) {
	return NetAddr(inet_makeaddr(net, lna), port);
}

in_addr NetAddr::NetMask(int subnet) {
	in_addr netmask;
	netmask.s_addr = htonl(INADDR_NONE << (32-subnet));
	return netmask;
}

unsigned long NetAddr::lna() const {
	return inet_lnaof(addrN());
}

unsigned long NetAddr::net() const {
	return inet_netof(addrN());
}

// a.b.c.d = 1.2.3.4
int NetAddr::octet(int idx) const {
	const unsigned long a = ntohl(addrN().s_addr);
	switch (idx) {
		case 1: return (int)((a >> 24) & 0xFF);
		case 2: return (int)((a >> 16) & 0xFF);
		case 3: return (int)((a >>  8) & 0xFF);
		case 4: return (int)((a      ) & 0xFF);
		default:
			Assert(false);
	}
	return 0;
}

void NetAddr::syncA() const {
	theAddrA = theAddrN.s_addr == INADDR_NONE ?
		string() : string(inet_ntoa(theAddrN));
}

void NetAddr::syncN() const {
	if (theAddrA.size())
		theAddrN.s_addr = inet_addr(theAddrA.c_str());
	else
		theAddrN.s_addr = INADDR_NONE;
}

bool NetAddr::sameButPort(const NetAddr &addr) const {
	if (theAddrN.s_addr != INADDR_NONE && addr.theAddrN.s_addr != INADDR_NONE)
		return theAddrN.s_addr == addr.theAddrN.s_addr;

	return addrA() == addr.addrA();
}

bool NetAddr::operator == (const NetAddr &addr) const {
	if (thePort != addr.thePort)
		return false;
	return sameButPort(addr);
}

bool NetAddr::operator < (const NetAddr &addr) const {
	if (thePort != addr.thePort)
		return thePort < addr.thePort;

	if (theAddrN.s_addr != INADDR_NONE && addr.theAddrN.s_addr != INADDR_NONE &&
		theAddrN.s_addr != addr.theAddrN.s_addr)
		return theAddrN.s_addr < addr.theAddrN.s_addr;

	return addrA() < addr.addrA();
}

bool NetAddr::isDomainName() const {
	const string &a = addrA();
	return !a.size() || !isdigit(a[0]) || !isdigit(a[a.size()-1]);
}

ostream &NetAddr::print(ostream &os) const {
	if (addrA().size())
		os << theAddrA;
	else
	if (thePort >= 0)
		os << '*';
	else
		os << "<none>";

	if (thePort >= 0)
		os << ':' << thePort;

	return os;
}


#if !defined(HAVE_INET_MAKEADDR) && defined(IN_CLASSA_HOST)

/* Formulate an Internet address from network + host. Used in
 * building addresses stored in the ifnet structure.
 * (c) The Regents of the University of California */
in_addr inet_makeaddr(unsigned long net, unsigned long host) {
	unsigned long addr;
	if (net < 128)
		addr = (net << IN_CLASSA_NSHIFT) | (host & IN_CLASSA_HOST);
	else 
	if (net < 65536)
		addr = (net << IN_CLASSB_NSHIFT) | (host & IN_CLASSB_HOST);
	else
	if (net < 16777216L)
		addr = (net << IN_CLASSC_NSHIFT) | (host & IN_CLASSC_HOST);
	else
		addr = net | host;
	addr = htonl(addr);
	return (in_addr&)addr;
}

unsigned long inet_lnaof(in_addr in) {
	const unsigned long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return (i & IN_CLASSA_HOST);
	else
	if (IN_CLASSB(i))
		return (i & IN_CLASSB_HOST);
	else
		return (i & IN_CLASSC_HOST);

}

unsigned long inet_netof(in_addr in) {
	const unsigned long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return ((i & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
	else
	if (IN_CLASSB(i))
		return ((i & IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
	else
		return ((i & IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
}

#endif
