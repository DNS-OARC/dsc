
SCRIPTS += refile-and-grok-node.sh
SCRIPTS += refile-and-grok.sh
SCRIPTS += remove-xmls.pl

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	test -d $(INSTALLDIR)/libexec || mkdir $(INSTALLDIR)/libexec
	install -m 755 $(SCRIPTS) $(INSTALLDIR)/libexec/
