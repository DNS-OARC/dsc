
EXTRACTORS += dsc-xml-extractor.pl

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -d -m 755 $(INSTALLDIR)/libexec/
	install -m 755 $(EXTRACTORS) $(INSTALLDIR)/libexec/
