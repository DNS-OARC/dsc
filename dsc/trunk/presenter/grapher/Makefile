
GRAPHERS != ls *-grapher.pl

all:

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	@test -d $(INSTALLDIR)/libexec/ || mkdir $(INSTALLDIR)/libexec/
	install -m 755 $(GRAPHERS) $(INSTALLDIR)/libexec/
	@test -d $(INSTALLDIR)/share/ || mkdir $(INSTALLDIR)/share/
	install -m 644 plots.mk $(INSTALLDIR)/share/
