all:
	(cd dsc; test -s Makefile || ./configure ; $(MAKE) $@)
	(cd cron; $(MAKE) $@)

install: all
	(cd dsc; $(MAKE) $@)
	(cd cron; $(MAKE) $@)

clean:
	(cd dsc; test -s Makefile && $(MAKE) $@ || true)

distclean:
	(cd dsc; test -s Makefile && $(MAKE) $@ || true)

.PHONY: all install clean distclean
