MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
    1458044657.dscdata.json \
    1458044657.dscdata.xml \
    1458044657.pcap.dist \
    pid.pcap.dist \
    pid.pid \
    1463589826.dscdata.xml

TESTS = test1.sh test2.sh test3.sh

test1.sh: 1458044657.pcap.dist

1458044657.pcap.dist: 1458044657.pcap
	ln -s "$(srcdir)/1458044657.pcap" 1458044657.pcap.dist

test2.sh: pid.pcap.dist

pid.pcap.dist: pid.pcap
	ln -s "$(srcdir)/pid.pcap" pid.pcap.dist

test3.sh: pid.pcap.dist

EXTRA_DIST = $(TESTS) \
    1458044657.conf \
    1458044657.pcap \
    1458044657.json_gold \
    1458044657.xml_gold \
    pid.conf \
    pid.pcap \
    statinter.conf \
    statinter2.conf \
    cnetmask.conf \
    cnetmask2.conf \
    cnetmask3.conf
