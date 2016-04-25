MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = 1458044657.dscdata.json \
    1458044657.dscdata.xml \
    1458044657.pcap.dist

TESTS = test1.sh

test1.sh: 1458044657.pcap.dist

1458044657.pcap.dist: 1458044657.pcap
	ln -s "$(srcdir)/1458044657.pcap" 1458044657.pcap.dist

EXTRA_DIST = $(TESTS) \
    1458044657.conf \
    1458044657.pcap \
    1458044657.json_gold \
    1458044657.xml_gold
