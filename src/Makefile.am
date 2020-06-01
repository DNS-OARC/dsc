MAINTAINERCLEANFILES = $(srcdir)/Makefile.in $(srcdir)/config.h.in
CLEANFILES = dsc.conf.sample dsc.1 dsc.conf.5

SUBDIRS = test

AM_CFLAGS = -I$(srcdir) \
  $(PTHREAD_CFLAGS) \
  $(libmaxminddb_CFLAGS) \
  $(libdnswire_CFLAGS) $(libuv_CFLAGS)

EXTRA_DIST = dsc.sh dsc.conf.sample.in dsc.1.in dsc.conf.5.in

etcdir = $(sysconfdir)/dsc
etc_DATA = dsc.conf.sample

bin_PROGRAMS = dsc
dsc_SOURCES = asn_index.c certain_qnames_index.c client_index.c \
  client_subnet_index.c compat.c config_hooks.c country_index.c daemon.c \
  dns_ip_version_index.c dns_message.c dns_protocol.c dns_source_port_index.c \
  do_bit_index.c edns_bufsiz_index.c edns_version_index.c hashtbl.c \
  idn_qname_index.c inX_addr.c ip_direction_index.c ip_proto_index.c \
  ip_version_index.c md_array.c md_array_json_printer.c \
  md_array_xml_printer.c msglen_index.c null_index.c opcode_index.c \
  parse_conf.c pcap.c qclass_index.c qname_index.c qnamelen_index.c \
  qr_aa_bits_index.c qtype_index.c query_classification_index.c rcode_index.c \
  rd_bit_index.c server_ip_addr_index.c tc_bit_index.c tld_index.c \
  transport_index.c xmalloc.c response_time_index.c \
  ext/base64.c ext/lookup3.c \
  pcap_layers/pcap_layers.c \
  pcap-thread/pcap_thread.c \
  dnstap.c
dist_dsc_SOURCES = asn_index.h base64.h certain_qnames_index.h client_index.h \
  client_subnet_index.h compat.h config_hooks.h country_index.h dataset_opt.h \
  dns_ip_version_index.h dns_message.h dns_protocol.h dns_source_port_index.h \
  do_bit_index.h edns_bufsiz_index.h edns_version_index.h geoip.h hashtbl.h \
  idn_qname_index.h inX_addr.h ip_direction_index.h ip_proto_index.h \
  ip_version_index.h md_array.h msglen_index.h null_index.h opcode_index.h \
  parse_conf.h pcap.h qclass_index.h qname_index.h qnamelen_index.h \
  qr_aa_bits_index.h qtype_index.h query_classification_index.h rcode_index.h \
  rd_bit_index.h server_ip_addr_index.h syslog_debug.h tc_bit_index.h \
  tld_index.h transport_index.h xmalloc.h response_time_index.h \
  pcap_layers/byteorder.h pcap_layers/pcap_layers.h \
  pcap-thread/pcap_thread.h \
  dnstap.h input_mode.h knowntlds.inc
dsc_LDADD = $(PTHREAD_LIBS) $(libmaxminddb_LIBS) \
  $(libdnswire_LIBS) $(libuv_LIBS)
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

dsc.1.html: dsc.1
	cat dsc.1 | groff -mandoc -Thtml > dsc.1.html

dsc.conf.5.html: dsc.conf.5
	cat dsc.conf.5 | groff -mandoc -Thtml > dsc.conf.5.html
