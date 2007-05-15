## Process this file with automake to produce Makefile.in

TESTS = \
	tests

check_PROGRAMS = \
	TestMaker \
	tests \
	SpeedTest \
	xmlgen

TestMaker_SOURCES = \
	TestMaker.cc

xmlgen_SOURCES = \
	xmlgen.c
	
SpeedTest_SOURCES = \
	SpeedTest.cc

EXTRA_DIST = \
	tests.cfg \
	SpeedTest.sh
	
nodist_tests_SOURCES = \
	tests.cc

MOSTLYCLEANFILES = \
	tests.cc

tests.cc: TestMaker $(srcdir)/tests.cfg
	./TestMaker < $(srcdir)/tests.cfg > ./tests.cc

LDADD = \
	-L$(top_builddir)/src -lHapy

INCLUDES = -I$(top_srcdir)/src/include -I$(top_builddir)/src/include

