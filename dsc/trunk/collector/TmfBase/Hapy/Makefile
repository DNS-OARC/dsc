#
# This is a generated file. Modifications are futile.
#

# src/Hapy/Makefile.  Generated from Makefile.in by configure.

.SUFFIXES:
.SUFFIXES: .h .cc .o .a
.PHONY: all all_subdirs clean clean_subdirs distclean distclean_subdirs install install_subdirs semiclean semiclean_subdirs

SHELL = /bin/sh



top_srcdir  = ../..
srcdir      = .
prefix      = /usr/local/coad
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin
datadir     = ${prefix}/share

INSTALL     = /usr/bin/install -c
INSTALL_PRG = ${INSTALL}
INSTALL_DTA = ${INSTALL} -m 644

STD_INCLUDE = 
LCL_INCLUDE = -I../.. -I..
INCLUDE     = $(LCL_INCLUDE) $(STD_INCLUDE)

DEFINES = -DHAVE_CONFIG_H

CPPFLAGS =  -I/usr/local/include $(INCLUDE) $(DEFINES)
CXX      = g++
CXXFLAGS = -g -O0 -Wall -Wwrite-strings -Woverloaded-virtual -pipe $(CPPFLAGS)
CCFLAGS  = $(CXXFLAGS)

RANLIB = ranlib
AR_R   = /usr/bin/ar r

LDFLAGS  =  -L/usr/local/lib
LIBS     = -lm 
EXEFLAGS = $(LDFLAGS) $(LIBS)

SUBDIRS = 

default: all

Buffer.cc: Buffer.h Parser.h

Buffer.h: Rules.h

Buffer.o: Buffer.cc
	$(CXX) -o Buffer.o -c Buffer.cc $(CXXFLAGS)

libHapy.a: Pree.o Parser.o Buffer.o Rule.o Rules.o RuleAlg.o RuleAlgs.o
	rm -f $@
	$(AR_R) $@ Pree.o Parser.o Buffer.o Rule.o Rules.o RuleAlg.o RuleAlgs.o
	$(RANLIB) $@

Parser.cc: Buffer.h Rules.h Parser.h

Parser.h: Pree.h Rules.h Buffer.h

Parser.o: Parser.cc
	$(CXX) -o Parser.o -c Parser.cc $(CXXFLAGS)

Pree.cc: Pree.h

Pree.o: Pree.cc
	$(CXX) -o Pree.o -c Pree.cc $(CXXFLAGS)

Result.h: Pree.h

Rule.cc: Buffer.h Parser.h RuleAlgs.h Rule.h

Rule.h: Result.h

Rule.o: Rule.cc
	$(CXX) -o Rule.o -c Rule.cc $(CXXFLAGS)

RuleAlg.cc: RuleAlg.h

RuleAlg.h: Result.h

RuleAlg.o: RuleAlg.cc
	$(CXX) -o RuleAlg.o -c RuleAlg.cc $(CXXFLAGS)

RuleAlgs.cc: Buffer.h Parser.h RuleAlgs.h

RuleAlgs.h: RuleAlg.h

RuleAlgs.o: RuleAlgs.cc
	$(CXX) -o RuleAlgs.o -c RuleAlgs.cc $(CXXFLAGS)

Rules.cc: Buffer.h Parser.h Rules.h RuleAlgs.h

Rules.h: Rule.h

Rules.o: Rules.cc
	$(CXX) -o Rules.o -c Rules.cc $(CXXFLAGS)

Test.cc: Parser.h Rules.h

Test: Test.o libHapy.a
	$(CXX) -o Test \
		Test.o libHapy.a ../xstd/libxstd.a \
		$(EXEFLAGS) 

Test_install: install_dirs Test
	$(INSTALL_PRG) Test $(bindir)

Test.o: Test.cc
	$(CXX) -o Test.o -c Test.cc $(CXXFLAGS)

TestMaker.cc: Parser.h Rules.h

TestMaker: TestMaker.o libHapy.a
	$(CXX) -o TestMaker \
		TestMaker.o libHapy.a ../xstd/libxstd.a \
		$(EXEFLAGS) 

TestMaker_install: install_dirs TestMaker
	$(INSTALL_PRG) TestMaker $(bindir)

TestMaker.o: TestMaker.cc
	$(CXX) -o TestMaker.o -c TestMaker.cc $(CXXFLAGS)

all: all_subdirs Test libHapy.a TestMaker

all_subdirs: 

clean: clean_subdirs 
	-rm -f *.o lib*.a *.core core libHapy.a Test TestMaker

clean_subdirs: 

distclean: distclean_subdirs 
	-rm -f Makefile config.cache config.status config.h config.log *.o lib*.a *.core core libHapy.a Test TestMaker
distclean_subdirs: 

install: install_subdirs Test libHapy.a TestMaker
install_dirs: 
	@for d in $(bindir)/; do \
		if test ! -d $$d; then echo mkdir -p $$d; mkdir -p $$d; fi; \
	done

install: Test_install TestMaker_install

install_subdirs: 

semiclean: semiclean_subdirs 
	-rm -f libHapy.a Test TestMaker *.core core

semiclean_subdirs: 

