## Process this file with automake to produce Makefile.in

check_PROGRAMS = \
	calc \
	calci \
	actions

calc_SOURCES = \
	calc.cc
	
calci_SOURCES = \
	calci.cc

actions_SOURCES = \
	actions.cc
	
LDADD = \
	-L$(top_builddir)/src -lHapy

AM_CPPFLAGS = -I$(top_srcdir)/src/include -I$(top_builddir)/src/include

site:
	SiteStylist config.ss . /tmp/hapy
	install -C -p calc.cc calci.cc actions.cc /tmp/hapy
	install -C -p benchmarking/xmlgen.diff /tmp/hapy/benchmarking/
