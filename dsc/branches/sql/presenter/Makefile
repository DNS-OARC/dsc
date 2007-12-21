INSTALLDIR=/usr/local/dsc

all clean:
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)

install:
	install -d -m 755 $(INSTALLDIR)/data/
	install -d -m 755 $(INSTALLDIR)/cache/
	install -d -m 755 $(INSTALLDIR)/etc/
	install -d -m 755 $(INSTALLDIR)/var/
	install -d -m 755 $(INSTALLDIR)/var/log/
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)

