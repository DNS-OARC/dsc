#
# This is a generated file. Modifications are futile.
#

.SUFFIXES:
.SUFFIXES: .h .cc .obj .lib

STD_INCLUDE = /I"\Program Files\Microsoft Visual Studio\VC98\INCLUDE"
LCL_INCLUDE = /I../.. /I..
INCLUDE     = $(LCL_INCLUDE) $(STD_INCLUDE)

DEFINES = /DHAVE_CONFIG_H

#CXX     = 
CXXFLAGS = /nologo /MLd /W3 /GX /Zi /O2 /DWIN32 /DNDEBUG /D_CONSOLE /D_MBCS /TP $(INCLUDE) $(DEFINES)
CCFLAGS  = $(CXXFLAGS)

RANLIB = echo
AR_R   = link.exe -lib
LINK   = link.exe

LDFLAGS  = /nologo
LIBS     = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib
EXEFLAGS = $(LDFLAGS) /subsystem:console /incremental:no /debug /machine:I386 $(LIBS)

SUBDIRS = 

default: all

Buffer.cc: Buffer.h Parser.h

Buffer.h: Rules.h

Buffer.obj: Buffer.cc
	$(CXX) /FoBuffer.obj -c Buffer.cc $(CXXFLAGS)

Hapy.lib: Pree.obj Parser.obj Buffer.obj Rule.obj Rules.obj RuleAlg.obj RuleAlgs.obj
	-erase /F $@
	$(AR_R) $(LDFLAGS) /out:$@ \
	Pree.obj Parser.obj Buffer.obj Rule.obj Rules.obj RuleAlg.obj RuleAlgs.obj

Parser.cc: Buffer.h Rules.h Parser.h

Parser.h: Pree.h Rules.h Buffer.h

Parser.obj: Parser.cc
	$(CXX) /FoParser.obj -c Parser.cc $(CXXFLAGS)

Pree.cc: Pree.h

Pree.obj: Pree.cc
	$(CXX) /FoPree.obj -c Pree.cc $(CXXFLAGS)

Result.h: Pree.h

Rule.cc: Buffer.h Parser.h RuleAlgs.h Rule.h

Rule.h: Result.h

Rule.obj: Rule.cc
	$(CXX) /FoRule.obj -c Rule.cc $(CXXFLAGS)

RuleAlg.cc: RuleAlg.h

RuleAlg.h: Result.h

RuleAlg.obj: RuleAlg.cc
	$(CXX) /FoRuleAlg.obj -c RuleAlg.cc $(CXXFLAGS)

RuleAlgs.cc: Buffer.h Parser.h RuleAlgs.h

RuleAlgs.h: RuleAlg.h

RuleAlgs.obj: RuleAlgs.cc
	$(CXX) /FoRuleAlgs.obj -c RuleAlgs.cc $(CXXFLAGS)

Rules.cc: Buffer.h Parser.h Rules.h RuleAlgs.h

Rules.h: Rule.h

Rules.obj: Rules.cc
	$(CXX) /FoRules.obj -c Rules.cc $(CXXFLAGS)

Test.cc: Parser.h Rules.h

Test.exe: Test.obj Hapy.lib
	$(LINK) $(EXEFLAGS) /out:$@ \
Test.obj Hapy.lib ../xstd/xstd.lib

Test.exe_install: install_dirs Test.exe
	@echo do not know how to install Test.exe on MS Windows

Test.obj: Test.cc
	$(CXX) /FoTest.obj -c Test.cc $(CXXFLAGS)

TestMaker.cc: Parser.h Rules.h

TestMaker.exe: TestMaker.obj Hapy.lib
	$(LINK) $(EXEFLAGS) /out:$@ \
TestMaker.obj Hapy.lib ../xstd/xstd.lib

TestMaker.exe_install: install_dirs TestMaker.exe
	@echo do not know how to install TestMaker.exe on MS Windows

TestMaker.obj: TestMaker.cc
	$(CXX) /FoTestMaker.obj -c TestMaker.cc $(CXXFLAGS)

all: all_subdirs Test.exe Hapy.lib TestMaker.exe

all_subdirs: 

clean: clean_subdirs 
	-erase /F *.obj *.lib core Hapy.lib Test.exe TestMaker.exe

clean_subdirs: 

distclean: distclean_subdirs 
	-erase /F *.obj *.lib core Hapy.lib Test.exe TestMaker.exe

distclean_subdirs: 

install: install_subdirs Test.exe Hapy.lib TestMaker.exe
install_dirs: 

install: Test.exe_install TestMaker.exe_install

install_subdirs: 

semiclean: semiclean_subdirs 
	-erase /F Hapy.lib Test.exe TestMaker.exe core

semiclean_subdirs: 

