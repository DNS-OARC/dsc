PROG=dsc
CFLAGS=-g -Wall -I TmfBase/Hapy/src/include
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
	msglen_index.o \
	client_ipv4_addr_index.o \
	client_ipv4_net_index.o \
	md_array_xml_printer.o \
	ip_direction_index.o \
	ip_proto_index.o \
	certain_qnames_index.o \
	ParseConfig.o \
	config_hooks.o

LIBHAPY=TmfBase/Hapy/src/.libs/libHapy.a
LIBS=\
	-lpcap \
	$(LIBHAPY)

all:  $(PROG)


$(PROG): $(OBJS) $(LIBS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

install: $(PROG)
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -m 755 $(PROG) $(INSTALLDIR)/bin/
	install -m 644 $(PROG).conf.sample $(INSTALLDIR)/etc/

clean:
	rm -f $(OBJS)
	rm -f $(PROG)
	cd TmfBase/Hapy; $(MAKE) $@

tar:
	cd /tmp; cvs checkout -d $(PROG)-$(TODAY) $(PROG) ; tar czvf $(PROG)-$(TODAY).tar.gz $(PROG)-$(TODAY)


ParseConfig.o: ParseConfig.cc $(LIBHAPY)

TmfBase/xstd/libxstd.a: TmfBase/xstd/Makefile
	@echo $@ is out of date
	cd TmfBase/xstd; $(MAKE) libxstd.a

TmfBase/xstd/Makefile:
	@echo $@ is out of date
	cd TmfBase/xstd; ./configure

$(LIBHAPY): TmfBase/Hapy/Makefile
	@echo $@ is out of date
	cd TmfBase/Hapy; $(MAKE) all

TmfBase/Hapy/Makefile:
	@echo $@ is out of date
	cd TmfBase/Hapy; ./configure


export:
	CVSROOT=`cat CVS/Root`; cd /tmp; cvs -d $$CVSROOT export -r HEAD -d dsc-`date +%Y%m%d` dsc
