
GRAPHERS += dsc-grapher.pl

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
