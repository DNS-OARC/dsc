
LIBEXECS += store-incoming-data.pl
LIBEXECS += put-file.pl
LIBEXECS += create-indexes.pl
LIBEXECS += drop-indexes.pl
LIBEXECS += import-flat-to-db.pl
LIBEXECS += init-db.pl

ETCFILES  += dsc-extractor.cfg.sample

INSTALLDIR=/usr/local/dsc

all:
	@for f in ${LIBEXECS} ; do echo "perl -c $$f"; perl -c $$f; done

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -d -m 755 $(INSTALLDIR)/libexec/
	install -m 755 $(LIBEXECS) $(INSTALLDIR)/libexec/

clean:
	
