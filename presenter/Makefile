INSTALLDIR=/usr/local/dsc
MKDIRS=data cache etc var var/log

all:
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)

install:
	@for f in ${MKDIRS} ; do \
	if test -d ${INSTALLDIR}/$$f ; then true ; else \
		echo install -d -m 755 $(INSTALLDIR)/$$f/ ; \
		install -d -m 755 $(INSTALLDIR)/$$f/ ; \
	fi \
	done
	(cd perllib; test -f Makefile || perl Makefile.PL ; $(MAKE) $@)
	(cd cron; $(MAKE) $@)
	(cd extractor; $(MAKE) $@)
	(cd grapher; $(MAKE) $@)

clean:
	(cd perllib; test -f Makefile && $(MAKE) $@ || true)
