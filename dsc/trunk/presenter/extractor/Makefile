
EXTRACTORS += certain_qnames_vs_qtype-extractor.pl
EXTRACTORS += client_subnet-extractor.pl
EXTRACTORS += client_subnet2-extractor.pl
EXTRACTORS += d0_bit-extractor.pl
EXTRACTORS += direction_vs_ipproto-extractor.pl
EXTRACTORS += edns_version-extractor.pl
EXTRACTORS += idn_qname-extractor.pl
EXTRACTORS += idn_vs_tld-extractor.pl
EXTRACTORS += ipv6_rsn_abusers-extractor.pl
EXTRACTORS += opcode-extractor.pl
EXTRACTORS += qtype-extractor.pl
EXTRACTORS += qtype_vs_qnamelen-extractor.pl
EXTRACTORS += qtype_vs_tld-extractor.pl
EXTRACTORS += rcode-extractor.pl
EXTRACTORS += rcode_vs_replylen-extractor.pl
EXTRACTORS += client_addr_vs_rcode-extractor.pl
EXTRACTORS += rd_bit-extractor.pl
EXTRACTORS += tlds-extractor.pl
EXTRACTORS += tables-cfg.pl

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -d -m 755 $(INSTALLDIR)/libexec/
	install -m 755 $(EXTRACTORS) $(INSTALLDIR)/libexec/
