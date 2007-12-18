
LIBEXECS += store-incoming-data.pl
LIBEXECS += put-file.pl

ETCFILES  += dsc-extractor.cfg.sample

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -d -m 755 $(INSTALLDIR)/libexec/
	install -m 755 $(LIBEXECS) $(INSTALLDIR)/libexec/

clean:
	
