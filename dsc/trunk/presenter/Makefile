INSTALLDIR=/usr/local/dsc

all clean:
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)

install:
	test -d $(INSTALLDIR)/data/ || install -d -m 755 $(INSTALLDIR)/data/
	test -d $(INSTALLDIR)/cache/ || install -d -m 755 $(INSTALLDIR)/cache/
	test -d $(INSTALLDIR)/etc/ || install -d -m 755 $(INSTALLDIR)/etc/
	test -d $(INSTALLDIR)/var/ || install -d -m 755 $(INSTALLDIR)/var/
	test -d $(INSTALLDIR)/var/log/ || install -d -m 755 $(INSTALLDIR)/var/log/
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)

