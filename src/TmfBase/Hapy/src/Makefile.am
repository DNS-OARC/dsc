## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = \
	libHapy.la

libHapy_la_SOURCES = \
	Algorithm.cc \
	Algorithms.cc \
	Area.cc \
	Assert.cc \
	Buffer.cc \
	Debugger.cc \
	First.cc \
	Optimizer.cc \
	Parser.cc \
	Pree.cc \
	PrettyPrint.cc \
	PreeFarm.cc \
	Result.cc \
	Rule.cc \
	RuleBase.cc \
	RuleId.cc \
	Rules.cc \
	SizeCalc.cc \
	Top.cc

include_pkgdir = ${prefix}/include/@PACKAGE@
include_pkg_HEADERS = \
	include/Hapy/Action.h \
	include/Hapy/ActionBase.h \
	include/Hapy/PtrAction.h \
	include/Hapy/MemAction.h \
	include/Hapy/Actions.h \
	\
	include/Hapy/Algorithm.h \
	include/Hapy/Algorithms.h \
	include/Hapy/Area.h \
	include/Hapy/Assert.h \
	include/Hapy/Buffer.h \
	include/Hapy/Debugger.h \
	include/Hapy/First.h \
	include/Hapy/IoStream.h \
	include/Hapy/IosFwd.h \
	include/Hapy/Iterator.h \
	include/Hapy/NumericLimits.h \
	include/Hapy/Optimizer.h \
	include/Hapy/Parser.h \
	include/Hapy/Pree.h \
	include/Hapy/PreeFarm.h \
	include/Hapy/PreeKids.h \
	include/Hapy/PrettyPrint.h \
	include/Hapy/Result.h \
	include/Hapy/Rule.h \
	include/Hapy/RuleCompFlags.h \
	include/Hapy/RuleBase.h \
	include/Hapy/RuleId.h \
	include/Hapy/RulePtr.h \
	include/Hapy/Rules.h \
	include/Hapy/String.h \
	include/Hapy/SizeCalc.h \
	include/Hapy/DeepPrint.h \
	include/Hapy/Top.h \
	include/Hapy/Version.h

nodist_include_pkg_HEADERS = \
	$(top_builddir)/src/include/Hapy/config.h

check_PROGRAMS = \
	example

example_SOURCES = \
	example.cc

example_LDADD = \
	libHapy.la @LIBOBJS@

DISTCLEANFILES = \
	$(nodist_include_pkg_HEADERS)

AM_CPPFLAGS = -I$(top_srcdir)/src/include -I$(top_builddir)/src/include
