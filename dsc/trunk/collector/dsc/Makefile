PROG=dsc
HAPY=../TmfBase/Hapy
CFLAGS=-g -Wall -I $(HAPY)/src/include
CPPFLAGS = ${CFLAGS}
TODAY != date +%Y%m%d
INSTALLDIR=/usr/local/dsc

OBJS=\
	base64.o \
	generic_counter.o \
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
	qname_index.o \
	msglen_index.o \
	client_ipv4_addr_index.o \
	client_ipv4_net_index.o \
	md_array_xml_printer.o \
	ip_direction_index.o \
	ip_proto_index.o \
	certain_qnames_index.o \
	query_classification_index.o \
	idn_qname_index.o \
	edns_version_index.o \
	do_bit_index.o \
	rd_bit_index.o \
	opcode_index.o \
	ParseConfig.o \
	config_hooks.o \
	hashtbl.o \
	xmalloc.o

LIBHAPY=$(HAPY)/src/.libs/libHapy.a
LIBS=\
	-lpcap \
	$(LIBHAPY)

all:  $(PROG)


$(PROG): $(OBJS) $(LIBHAPY)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

install: $(PROG)
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -d -m 755 $(INSTALLDIR)/bin/
	install -d -m 755 $(INSTALLDIR)/etc/
	install -d -m 755 $(INSTALLDIR)/var/
	install -d -m 755 $(INSTALLDIR)/var/log/
	install -m 755 $(PROG) $(INSTALLDIR)/bin/
	install -m 644 $(PROG).conf.sample $(INSTALLDIR)/etc/

clean:
	rm -f $(OBJS)
	rm -f $(PROG)
	cd $(HAPY); $(MAKE) $@

tar:
	cd /tmp; cvs checkout -d $(PROG)-$(TODAY) $(PROG) ; tar czvf $(PROG)-$(TODAY).tar.gz $(PROG)-$(TODAY)


ParseConfig.o: ParseConfig.cc $(LIBHAPY)

$(LIBHAPY): $(HAPY)/Makefile
	@echo $@ is out of date
	cd $(HAPY); $(MAKE) all

$(HAPY)/Makefile:
	@echo $@ is out of date
	cd $(HAPY); ./configure


export:
	CVSROOT=`cat CVS/Root`; cd /tmp; cvs -d $$CVSROOT export -r HEAD -d dsc-`date +%Y%m%d` dsc
