ACLOCAL_AMFLAGS = -I m4 -I src/pcap-thread/m4

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in \
    $(srcdir)/src/config.h.in~ \
    $(srcdir)/configure

SUBDIRS = src

dist_doc_DATA = README.md CHANGES LICENSE UPGRADE.md

EXTRA_DIST = m4 doc autogen.sh .clang-format fmt.sh

test: check
