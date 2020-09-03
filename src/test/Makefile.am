MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
  1458044657.dscdata.json 1458044657.dscdata.xml 1458044657.pcap.dist \
  pid.pcap.dist pid.pid \
  1463589826.dscdata.xml \
  test.dnstap.dist 1573730567.dscdata.xml \
  mmdb.conf.run \
  dns6.conf.run dns6.pcap.dist 1543333920.dscdata.xml

EXTRA_DIST =

TESTS = test1.sh test2.sh test3.sh test4.sh test6.sh test7.sh

if USE_DNSTAP
TESTS += test5.sh
else
EXTRA_DIST += test5.sh
endif

test1.sh: 1458044657.pcap.dist

1458044657.pcap.dist: 1458044657.pcap
	ln -s "$(srcdir)/1458044657.pcap" 1458044657.pcap.dist

test2.sh: pid.pcap.dist

pid.pcap.dist: pid.pcap
	ln -s "$(srcdir)/pid.pcap" pid.pcap.dist

test3.sh: pid.pcap.dist

test4.sh: 1458044657.pcap.dist

test.dnstap.dist: test.dnstap
	ln -s "$(srcdir)/test.dnstap" test.dnstap.dist

test5.sh: test.dnstap.dist

test6.sh: 1458044657.pcap.dist

test7.sh: dns6.pcap.dist

dns6.pcap.dist: dns6.pcap
	ln -s "$(srcdir)/dns6.pcap" dns6.pcap.dist

EXTRA_DIST += $(TESTS) \
  1458044657.conf 1458044657.pcap 1458044657.json_gold 1458044657.xml_gold \
  pid.conf pid.pcap \
  statinter.conf statinter2.conf \
  cnetmask.conf cnetmask2.conf cnetmask3.conf \
  parseconf.conf parseconf2.conf \
  response_time.conf response_time.gold \
  response_time2.conf response_time2.gold \
  response_time3.conf response_time3.gold \
  test.dnstap 1573730567.conf 1573730567.gold \
  mmdb.conf mmdb.gold \
  dns6.pcap dns6.conf dns6.gold
