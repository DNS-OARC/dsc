
GRAPHERS += certain_qnames2-grapher.pl
GRAPHERS += certain_qnames_vs_qtype-grapher.pl
GRAPHERS += client_subnet-grapher.pl
GRAPHERS += client_subnet2-by-class-grapher.pl
GRAPHERS += client_subnet2-grapher.pl
GRAPHERS += d0_bit-grapher.pl
GRAPHERS += edns_version-grapher.pl
GRAPHERS += idn_qname-grapher.pl
GRAPHERS += ipproto-grapher.pl
GRAPHERS += ipv6_rsn_abusers-grapher.pl
GRAPHERS += opcode-grapher.pl
GRAPHERS += opcode2-grapher.pl
GRAPHERS += qtype-grapher.pl
GRAPHERS += qtype2-grapher.pl
GRAPHERS += qtype_vs_qnamelen-grapher.pl
GRAPHERS += qtype_vs_tld-grapher.pl
GRAPHERS += qtype_vs_tld2-grapher.pl
GRAPHERS += query_attrs-grapher.pl
GRAPHERS += rcode-grapher.pl
GRAPHERS += rcode_vs_replylen-grapher.pl
GRAPHERS += server-qcounts-grapher.pl
GRAPHERS += tlds-grapher.pl
GRAPHERS += dsc-grapher.pl
GRAPHERS += plots-cfg.pl

GRAPHERS += whois.pl
GRAPHERS += hostinfo.pl

CONFIGS  += dsc-grapher.cfg.sample

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "${INSTALLDIR}" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	@test -d ${INSTALLDIR}/libexec/ || mkdir ${INSTALLDIR}/libexec/
	install -m 755 ${GRAPHERS} ${INSTALLDIR}/libexec/
	@test -d ${INSTALLDIR}/share/ || mkdir ${INSTALLDIR}/share/
	install -m 644 *.mk ${INSTALLDIR}/share/
	@test -d ${INSTALLDIR}/etc/ || mkdir ${INSTALLDIR}/etc/
	install -m 644 ${CONFIGS} ${INSTALLDIR}/etc/
