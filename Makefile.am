MAINTAINERCLEANFILES = $(srcdir)/Makefile.in \
    $(srcdir)/src/config.h.in~ \
    $(srcdir)/configure

SUBDIRS = src cron

dist_doc_DATA = README.md CHANGES LICENSE UPGRADE.md

EXTRA_DIST = doc ports autogen.sh

test: check
