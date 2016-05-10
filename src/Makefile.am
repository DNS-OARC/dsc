MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
CLEANFILES = dsc.conf.sample dsc.1 dsc.conf.5

SUBDIRS = test

AM_CFLAGS = -I$(srcdir) \
    -Wall \
    -DUSE_IPV6=1

AM_CPPFLAGS = -I$(srcdir) \
    -ITmfBase/Hapy/src/include \
    -I$(srcdir)/TmfBase/Hapy/src/include \
    -Wall \
    -DUSE_IPV6=1

EXTRA_DIST = TmfBase \
    dsc.sh \
    dsc.conf.sample.in \
    dsc.1.in \
    dsc.conf.5.in

etcdir = $(sysconfdir)/dsc
etc_DATA = dsc.conf.sample

bin_PROGRAMS = dsc
dsc_SOURCES = base64.c \
    generic_counter.c \
    pcap_layers/pcap_layers.c \
    pcap.c \
    ncap.c \
    dns_protocol.c \
    dns_message.c \
    daemon.c \
    md_array.c \
    null_index.c \
    qtype_index.c \
    qclass_index.c \
    tld_index.c \
    country_index.c \
    asn_index.c \
    rcode_index.c \
    qnamelen_index.c \
    qname_index.c \
    msglen_index.c \
    client_ip_addr_index.c \
    client_ip_net_index.c \
    server_ip_addr_index.c \
    md_array_xml_printer.c \
    md_array_json_printer.c \
    ip_direction_index.c \
    ip_proto_index.c \
    ip_version_index.c \
    certain_qnames_index.c \
    query_classification_index.c \
    idn_qname_index.c \
    edns_version_index.c \
    edns_bufsiz_index.c \
    do_bit_index.c \
    rd_bit_index.c \
    tc_bit_index.c \
    qr_aa_bits_index.c \
    opcode_index.c \
    transport_index.c \
    dns_ip_version_index.c \
    dns_source_port_index.c \
    ParseConfig.cc \
    config_hooks.c \
    hashtbl.c \
    lookup3.c \
    xmalloc.c \
    inX_addr.c
dist_dsc_SOURCES = base64.h \
    byteorder.h \
    certain_qnames_index.h \
    client_ip_addr_index.h \
    client_ip_net_index.h \
    country_index.h \
    asn_index.h \
    dataset_opt.h \
    dns_ip_version_index.h \
    dns_message.h \
    dns_source_port_index.h \
    do_bit_index.h \
    edns_bufsiz_index.h \
    edns_version_index.h \
    generic_counter.h \
    hashtbl.h \
    idn_qname_index.h \
    inX_addr.h \
    ip_direction_index.h \
    ip_proto_index.h \
    ip_version_index.h \
    md_array.h \
    msglen_index.h \
    ncap.h \
    null_index.h \
    opcode_index.h \
    pcap.h \
    pcap_layers.h \
    qclass_index.h \
    qname_index.h \
    qnamelen_index.h \
    qr_aa_bits_index.h \
    qtype_index.h \
    query_classification_index.h \
    rcode_index.h \
    rd_bit_index.h \
    server_ip_addr_index.h \
    syslog_debug.h \
    tc_bit_index.h \
    tld_index.h \
    transport_index.h \
    xmalloc.h \
    pcap_layers/byteorder.h \
    pcap_layers/pcap_layers.h
dsc_LDADD = TmfBase/Hapy/src/.libs/libHapy.a
man1_MANS = dsc.1
man5_MANS = dsc.conf.5

dsc.conf.sample: dsc.conf.sample.in Makefile
	sed -e 's,[@]DSC_PID_FILE[@],$(DSC_PID_FILE),g' \
        -e 's,[@]DSC_DATA_DIR[@],$(DSC_DATA_DIR),g' \
        < $(srcdir)/dsc.conf.sample.in > dsc.conf.sample

dsc.1: dsc.1.in Makefile
	sed -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
        -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
        -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
        -e 's,[@]etcdir[@],$(etcdir),g' \
        < $(srcdir)/dsc.1.in > dsc.1

dsc.conf.5: dsc.conf.5.in Makefile
	sed -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
        -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
        -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
        -e 's,[@]etcdir[@],$(etcdir),g' \
        < $(srcdir)/dsc.conf.5.in > dsc.conf.5

dist-hook:
	cd $(distdir)/TmfBase/Hapy && $(MAKE) $(AM_MAKEFLAGS) distclean

TmfBase/Hapy/src/.libs/libHapy.a:
	cd TmfBase/Hapy && $(MAKE) $(AM_MAKEFLAGS) all

clean-local:
	cd TmfBase/Hapy && $(MAKE) $(AM_MAKEFLAGS) clean

maintainer-clean-local:
	cd TmfBase/Hapy && $(MAKE) $(AM_MAKEFLAGS) maintainer-clean
