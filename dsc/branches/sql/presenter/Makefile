INSTALLDIR=/usr/local/dsc

all clean:
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)

install:
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)
	# note ideally these need to be owned by the httpd uid
	install -d -m 755 $(INSTALLDIR)/data/
	install -d -m 755 $(INSTALLDIR)/cache/
	install -d -m 755 $(INSTALLDIR)/var/
	install -d -m 755 $(INSTALLDIR)/var/log/

