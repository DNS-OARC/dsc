MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

CLEANFILES = test*.log test*.trs \
  1458044657.dscdata.json 1458044657.dscdata.xml 1458044657.pcap.dist \
  pid.pcap.dist pid.pid \
  1463589826.dscdata.xml \
  test.dnstap.dist 1573730567.dscdata.xml \
  mmdb.conf.run \
  dns6.conf.run dns6.pcap.dist 1543333920.dscdata.xml \
  dnso1tcp.pcap.dist 1515583363.dscdata.xml \
  test9.out \
  test10.conf test10.out \
  test11.out* test11.pid knowntlds.txt.dist \
  test12.out \
  1458044657.tld_list.dist \
  tld_list.dat \
  dotdoh.dnstap.dist 1643283234.dscdata.xml \
  test13.conf \
  test_285.pcap.dist test_285.tldlist.dist 1683879752.dscdata.xml \
  h-root-aaa.pcap-dist 1688028728.dscdata.xml

EXTRA_DIST =

TESTS = test1.sh test2.sh test3.sh test4.sh test6.sh test7.sh test8.sh \
  test9.sh test10.sh test11.sh test12.sh test_dnstap_unixsock.sh \
  test_dnstap_tcp.sh test_pslconv.sh test_encrypted.sh test13.sh \
  test_285.sh test_291.sh

if USE_DNSTAP
TESTS += test5.sh
else
EXTRA_DIST += test5.sh
endif

test1.sh: 1458044657.pcap.dist 1458044657.tld_list.dist

1458044657.pcap.dist: 1458044657.pcap
	ln -s "$(srcdir)/1458044657.pcap" 1458044657.pcap.dist

1458044657.tld_list.dist: 1458044657.tld_list
	ln -s "$(srcdir)/1458044657.tld_list" 1458044657.tld_list.dist

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

test8.sh: dnso1tcp.pcap.dist

dnso1tcp.pcap.dist: dnso1tcp.pcap
	ln -s "$(srcdir)/dnso1tcp.pcap" dnso1tcp.pcap.dist

test9.sh: test.dnstap.dist 1458044657.pcap.dist knowntlds.txt.dist

test11.sh: 1458044657.pcap.dist

test12.sh: knowntlds.txt.dist 1458044657.pcap.dist

knowntlds.txt.dist: knowntlds.txt
	ln -s "$(srcdir)/knowntlds.txt" knowntlds.txt.dist

test_encrypted.sh: dotdoh.dnstap.dist

dotdoh.dnstap.dist: dotdoh.dnstap
	ln -s "$(srcdir)/dotdoh.dnstap" dotdoh.dnstap.dist

test13.sh: 1458044657.pcap.dist 1458044657.tld_list.dist

test_285.pcap.dist: test_285.pcap
	ln -s "$(srcdir)/test_285.pcap" test_285.pcap.dist

test_285.tldlist.dist: test_285.tldlist
	ln -s "$(srcdir)/test_285.tldlist" test_285.tldlist.dist

test_285.sh: test_285.pcap.dist test_285.tldlist.dist

h-root-aaa.pcap-dist: h-root-aaa.pcap
	ln -s "$(srcdir)/h-root-aaa.pcap" h-root-aaa.pcap-dist

test_291.sh: h-root-aaa.pcap-dist

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
  dns6.pcap dns6.conf dns6.gold \
  dnso1tcp.pcap dnso1tcp.conf dnso1tcp.gold \
  test9/bpf_vlan_tag_order.conf test9/bpf_vlan_tag_order.grep test9/dataset_already_exists.conf test9/dataset_already_exists.grep test9/dataset_response_time.conf test9/dataset_response_time.grep test9/dns_port.conf test9/dns_port.grep test9/dnstap_input_mode_set.conf test9/dnstap_input_mode_set.grep test9/dnstap_invalid_port_tcp.conf test9/dnstap_invalid_port_tcp.grep test9/dnstap_invalid_port_udp.conf test9/dnstap_invalid_port_udp.grep test9/dnstap_only_one.conf test9/dnstap_only_one.grep test9/geoip_backend2.conf test9/geoip_backend.conf test9/geoip.conf test9/interface_input_mode_set.conf test9/interface_input_mode_set.grep test9/knowntlds2.conf test9/knowntlds2.grep test9/knowntlds.conf test9/knowntlds.grep test9/output_format.conf test9/output_format.grep test9/response_time_full_mode.conf test9/response_time_full_mode.grep test9/response_time_max_sec_mode.conf test9/response_time_max_sec_mode.grep test9/response_time_mode.conf test9/response_time_mode.grep test9/run_dir.conf test9/run_dir.grep \
  test11.conf test11.gold \
  test12.conf knowntlds.txt \
  dnstap_unixsock.conf dnstap_tcp.conf \
  1458044657.tld_list \
  public_suffix_list.dat tld_list.dat.gold \
  dnstap_encrypted.conf dnstap_encrypted.gold dotdoh.dnstap \
  test_285.pcap test_285.conf test_285.tldlist test_285.xml_gold \
  h-root-aaa.pcap test_291.conf test_291.xml_gold
