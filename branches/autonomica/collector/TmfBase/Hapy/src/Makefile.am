## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = \
	libHapy.la

libHapy_la_SOURCES = \
	Algorithm.cc \
	Algorithms.cc \
	Area.cc \
	Assert.cc \
	Buffer.cc \
	Parser.cc \
	Pree.cc \
	PrettyPrint.cc \
	PreeKids.cc \
	Result.cc \
	Rule.cc \
	RuleBase.cc \
	RuleId.cc \
	Rules.cc

include_pkgdir = ${prefix}/include/@PACKAGE@
include_pkg_HEADERS = \
	include/Hapy/Action.h \
	include/Hapy/Algorithm.h \
	include/Hapy/Algorithms.h \
	include/Hapy/Area.h \
	include/Hapy/Assert.h \
	include/Hapy/Buffer.h \
	include/Hapy/FunAction.h \
	include/Hapy/IoStream.h \
	include/Hapy/IosFwd.h \
	include/Hapy/NumericLimits.h \
	include/Hapy/Parser.h \
	include/Hapy/Pree.h \
	include/Hapy/PreeKids.h \
	include/Hapy/PrettyPrint.h \
	include/Hapy/Result.h \
	include/Hapy/Rule.h \
	include/Hapy/RuleBase.h \
	include/Hapy/RuleId.h \
	include/Hapy/RulePtr.h \
	include/Hapy/Rules.h \
	include/Hapy/HapyString.h \
	include/Hapy/Version.h

nodist_include_pkg_HEADERS = \
	$(top_builddir)/src/include/Hapy/config.h

check_PROGRAMS = \
	example

example_SOURCES = \
	example.cc

example_LDADD = \
	libHapy.la @LIBOBJS@


INCLUDES = -I$(top_srcdir)/src/include -I$(top_builddir)/src/include
