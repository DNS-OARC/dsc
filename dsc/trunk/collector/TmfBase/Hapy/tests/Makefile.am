## Process this file with automake to produce Makefile.in

TESTS = \
	tests-sp \
	tests-ep

check_PROGRAMS = \
	TestMaker \
	tests-sp \
	tests-ep \
	\
	SpeedTest \
	xmlgen

TestMaker_SOURCES = \
	TestMaker.cc \
	TestCase.h

xmlgen_SOURCES = \
	xmlgen.c
	
SpeedTest_SOURCES = \
	SpeedTest.cc

EXTRA_DIST = \
	tests.cfg \
	SpeedTest.sh
	
nodist_tests_sp_SOURCES = \
	tests-sp.cc

nodist_tests_ep_SOURCES = \
	tests-ep.cc

MOSTLYCLEANFILES = \
	tests-sp.cc \
	tests-ep.cc

tests-sp.cc: TestMaker TestCase.h $(srcdir)/tests.cfg
	./TestMaker --share-parsers < $(srcdir)/tests.cfg > ./tests-sp.cc

tests-ep.cc: TestMaker TestCase.h $(srcdir)/tests.cfg
	./TestMaker --encapsulate-parsers < $(srcdir)/tests.cfg > ./tests-ep.cc

LDADD = \
	-L$(top_builddir)/src -lHapy

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/include -I$(top_builddir)/src/include \
	-I$(top_srcdir)/tests

