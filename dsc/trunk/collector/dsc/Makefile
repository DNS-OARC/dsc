PROG=dsc
CFLAGS=-g -Wall -I TmfBase
TODAY != date +%Y%m%d

OBJS=\
	base64.o \
	generic_counter.o \
	string_counter.o \
	pcap.o \
	dns_protocol.o \
	dns_message.o \
	ip_message.o \
	daemon.o \
	md_array.o \
	null_index.o \
	qtype_index.o \
	qclass_index.o \
	tld_index.o \
	rcode_index.o \
	qnamelen_index.o \
	client_ipv4_addr_index.o \
	client_ipv4_net_index.o \
	md_array_xml_printer.o \
	ip_direction_index.o \
	ip_proto_index.o \
	ParseConfig.o \
	config_hooks.o

LIBS=\
	-lpcap \
	TmfBase/Hapy/libHapy.a \
	TmfBase/xstd/libxstd.a

all:  $(PROG)


$(PROG): $(OBJS) $(LIBS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -m 755 $(PROG) $(INSTALLDIR)/bin
	install -m 644 $(PROG).conf.sample $(INSTALLDIR)/etc

clean:
	rm -f $(OBJS)
	rm -f $(PROG)
	cd TmfBase/xstd; $(MAKE) $@
	cd TmfBase/Hapy; $(MAKE) $@

tar:
	cd /tmp; cvs checkout -d $(PROG)-$(TODAY) $(PROG) ; tar czvf $(PROG)-$(TODAY).tar.gz $(PROG)-$(TODAY)

TmfBase/xstd/libxstd.a: TmfBase/xstd/Makefile
	@echo $@ is out of date
	cd TmfBase/xstd; $(MAKE)

TmfBase/xstd/Makefile:
	@echo $@ is out of date
	cd TmfBase/xstd; ./configure

TmfBase/Hapy/libHapy.a: TmfBase/Hapy/Makefile TmfBase/xstd/libxstd.a
	@echo $@ is out of date
	cd TmfBase/Hapy; $(MAKE)


# base64.c client_ipv4_addr_index.c client_ipv4_net_index.c daemon.c dns_message.c dns_protocol.c generic_counter.c md_array.c md_array_xml_printer.c null_index.c pcap.c qclass_index.c qnamelen_index.c qtype_index.c rcode_index.c string_counter.c tld_index.c
base64.o: base64.c /usr/include/stdlib.h /usr/include/sys/cdefs.h \
 /usr/include/machine/ansi.h /usr/include/string.h
client_ipv4_addr_index.o: client_ipv4_addr_index.c \
 /usr/include/stdlib.h /usr/include/sys/cdefs.h \
 /usr/include/machine/ansi.h /usr/include/assert.h \
 /usr/include/string.h dns_message.h /usr/include/sys/types.h \
 /usr/include/sys/inttypes.h /usr/include/machine/types.h \
 /usr/include/machine/endian.h /usr/include/sys/socket.h \
 /usr/include/machine/param.h /usr/include/netinet/in.h \
 /usr/include/netinet6/in6.h /usr/include/arpa/inet.h \
 /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/_posix.h \
 md_array.h
client_ipv4_net_index.o: client_ipv4_net_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/string.h dns_message.h \
 /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h
daemon.o: daemon.c /usr/include/stdio.h /usr/include/sys/cdefs.h \
 /usr/include/machine/ansi.h /usr/include/stdlib.h \
 /usr/include/string.h /usr/include/unistd.h /usr/include/sys/types.h \
 /usr/include/sys/inttypes.h /usr/include/machine/types.h \
 /usr/include/machine/endian.h /usr/include/sys/unistd.h \
 /usr/include/sys/_posix.h /usr/include/sys/wait.h dns_message.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 pcap.h
dns_message.o: dns_message.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/stdio.h dns_message.h \
 /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h null_index.h qtype_index.h \
 qclass_index.h tld_index.h rcode_index.h client_ipv4_addr_index.h \
 client_ipv4_net_index.h
dns_protocol.o: dns_protocol.c /usr/include/unistd.h \
 /usr/include/sys/cdefs.h /usr/include/sys/types.h \
 /usr/include/sys/inttypes.h /usr/include/machine/ansi.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/unistd.h /usr/include/sys/_posix.h \
 /usr/include/stdlib.h /usr/include/string.h /usr/include/sys/time.h \
 /usr/include/time.h /usr/include/assert.h /usr/include/ctype.h \
 /usr/include/runetype.h string_counter.h dns_message.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h
generic_counter.o: generic_counter.c generic_counter.h
md_array.o: md_array.c /usr/include/stdlib.h /usr/include/sys/cdefs.h \
 /usr/include/machine/ansi.h /usr/include/assert.h \
 /usr/include/stdio.h dns_message.h /usr/include/sys/types.h \
 /usr/include/sys/inttypes.h /usr/include/machine/types.h \
 /usr/include/machine/endian.h /usr/include/sys/socket.h \
 /usr/include/machine/param.h /usr/include/netinet/in.h \
 /usr/include/netinet6/in6.h /usr/include/arpa/inet.h \
 /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/_posix.h \
 md_array.h pcap.h
md_array_xml_printer.o: md_array_xml_printer.c /usr/include/stdio.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/stdlib.h /usr/include/string.h /usr/include/assert.h \
 dns_message.h /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h pcap.h base64.h
null_index.o: null_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h dns_message.h /usr/include/sys/types.h \
 /usr/include/sys/inttypes.h /usr/include/machine/types.h \
 /usr/include/machine/endian.h /usr/include/sys/socket.h \
 /usr/include/machine/param.h /usr/include/netinet/in.h \
 /usr/include/netinet6/in6.h /usr/include/arpa/inet.h \
 /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/_posix.h
pcap.o: pcap.c /usr/include/sys/types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/inttypes.h /usr/include/machine/ansi.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/_posix.h \
 /usr/include/sys/stat.h /usr/include/netinet/in.h \
 /usr/include/netinet6/in6.h /usr/include/pcap.h \
 /usr/include/net/bpf.h /usr/include/stdio.h /usr/include/signal.h \
 /usr/include/sys/signal.h /usr/include/machine/signal.h \
 /usr/include/machine/trap.h /usr/include/sys/ucontext.h \
 /usr/include/machine/ucontext.h /usr/include/stdlib.h \
 /usr/include/string.h /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/ctype.h /usr/include/runetype.h /usr/include/assert.h \
 /usr/include/arpa/inet.h /usr/include/arpa/nameser.h \
 /usr/include/arpa/nameser_compat.h /usr/include/sys/socket.h \
 /usr/include/machine/param.h /usr/include/net/if_arp.h \
 /usr/include/net/if.h /usr/include/netinet/if_ether.h \
 /usr/include/net/ethernet.h /usr/include/netinet/in_systm.h \
 /usr/include/netinet/ip.h /usr/include/netinet/udp.h dns_message.h \
 pcap.h
qclass_index.o: qclass_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/stdio.h dns_message.h \
 /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h
qnamelen_index.o: qnamelen_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/stdio.h /usr/include/string.h \
 dns_message.h /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h
qtype_index.o: qtype_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/stdio.h dns_message.h \
 /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h
rcode_index.o: rcode_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/stdio.h dns_message.h \
 /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h
string_counter.o: string_counter.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/unistd.h /usr/include/sys/types.h \
 /usr/include/sys/inttypes.h /usr/include/machine/types.h \
 /usr/include/machine/endian.h /usr/include/sys/unistd.h \
 /usr/include/sys/_posix.h /usr/include/memory.h /usr/include/string.h \
 string_counter.h generic_counter.h
tld_index.o: tld_index.c /usr/include/stdlib.h \
 /usr/include/sys/cdefs.h /usr/include/machine/ansi.h \
 /usr/include/assert.h /usr/include/stdio.h /usr/include/string.h \
 dns_message.h /usr/include/sys/types.h /usr/include/sys/inttypes.h \
 /usr/include/machine/types.h /usr/include/machine/endian.h \
 /usr/include/sys/socket.h /usr/include/machine/param.h \
 /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
 /usr/include/arpa/inet.h /usr/include/sys/time.h /usr/include/time.h \
 /usr/include/sys/_posix.h md_array.h
