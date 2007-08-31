
LIBEXECFILES += dsc-grapher.pl.sample

ETCFILES  += dsc-grapher.cfg.sample

SHAREHTMLFILES	+= plot.page
SHAREHTMLFILES	+= icons/1downarrow.png
SHAREHTMLFILES	+= icons/1leftarrow.png
SHAREHTMLFILES	+= icons/1rightarrow.png
SHAREHTMLFILES	+= icons/1uparrow.png
SHAREHTMLFILES	+= icons/2downarrow.png
SHAREHTMLFILES	+= icons/2leftarrow.png
SHAREHTMLFILES	+= icons/2rightarrow.png
SHAREHTMLFILES	+= icons/2uparrow.png

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "${INSTALLDIR}" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	@test -d ${INSTALLDIR}/libexec/ || mkdir ${INSTALLDIR}/libexec/
	install -m 755 ${LIBEXECFILES} ${INSTALLDIR}/libexec/
	test -f ${INSTALLDIR}/libexec/dsc-grapher.pl || install -m 755 dsc-grapher.pl.sample ${INSTALLDIR}/libexec/dsc-grapher.pl
	install -m 755 ${LIBEXECFILES} ${INSTALLDIR}/libexec/
	@test -d ${INSTALLDIR}/etc/ || mkdir ${INSTALLDIR}/etc/
	install -m 644 ${ETCFILES} ${INSTALLDIR}/etc/
	@test -d ${INSTALLDIR}/share/ || mkdir ${INSTALLDIR}/share/
	@test -d ${INSTALLDIR}/share/html || mkdir ${INSTALLDIR}/share/html/
	install -m 644 ${SHAREHTMLFILES} ${INSTALLDIR}/share/html/

clean:
	
