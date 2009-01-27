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

Algorithm.cc: Assert.h Algorithm.h

Algorithm.h: Result.h

Algorithm.obj: Algorithm.cc
	$(CXX) /FoAlgorithm.obj -c Algorithm.cc $(CXXFLAGS)

Algorithms.cc: Assert.h Buffer.h Parser.h Algorithms.h

Algorithms.h: Algorithm.h

Algorithms.obj: Algorithms.cc
	$(CXX) /FoAlgorithms.obj -c Algorithms.cc $(CXXFLAGS)

Assert.cc: Assert.h

Assert.obj: Assert.cc
	$(CXX) /FoAssert.obj -c Assert.cc $(CXXFLAGS)

Buffer.cc: Assert.h Buffer.h Parser.h

Buffer.h: Rules.h

Buffer.obj: Buffer.cc
	$(CXX) /FoBuffer.obj -c Buffer.cc $(CXXFLAGS)

Hapy.lib: Pree.obj PreeKids.obj Parser.obj Buffer.obj Rule.obj Rules.obj Algorithm.obj Algorithms.obj Assert.obj
	-erase /F $@
	$(AR_R) $(LDFLAGS) /out:$@ \
	Pree.obj PreeKids.obj Parser.obj Buffer.obj Rule.obj Rules.obj Algorithm.obj Algorithms.obj Assert.obj

Parser.cc: Assert.h Buffer.h Rules.h Parser.h

Parser.h: Pree.h Rules.h Buffer.h

Parser.obj: Parser.cc
	$(CXX) /FoParser.obj -c Parser.cc $(CXXFLAGS)

Pree.cc: Assert.h Pree.h

Pree.h: PreeKids.h

Pree.obj: Pree.cc
	$(CXX) /FoPree.obj -c Pree.cc $(CXXFLAGS)

PreeKids.cc: Pree.h PreeKids.h

PreeKids.obj: PreeKids.cc
	$(CXX) /FoPreeKids.obj -c PreeKids.cc $(CXXFLAGS)

Result.h: Pree.h

Rule.cc: Assert.h Buffer.h Parser.h Algorithms.h Rule.h

Rule.h: Result.h

Rule.obj: Rule.cc
	$(CXX) /FoRule.obj -c Rule.cc $(CXXFLAGS)

Rules.cc: Assert.h Buffer.h Parser.h Rules.h Algorithms.h

Rules.h: Rule.h

Rules.obj: Rules.cc
	$(CXX) /FoRules.obj -c Rules.cc $(CXXFLAGS)

TestMaker.cc: Parser.h Rules.h Assert.h

TestMaker.exe: TestMaker.obj Hapy.lib
	$(LINK) $(EXEFLAGS) /out:$@ \
TestMaker.obj Hapy.lib

TestMaker.exe_install: install_dirs TestMaker.exe
	@echo do not know how to install TestMaker.exe on MS Windows

TestMaker.obj: TestMaker.cc
	$(CXX) /FoTestMaker.obj -c TestMaker.cc $(CXXFLAGS)

example.cc: Parser.h Rules.h

example.exe: example.obj Hapy.lib
	$(LINK) $(EXEFLAGS) /out:$@ \
example.obj Hapy.lib

example.exe_install: install_dirs example.exe
	@echo do not know how to install example.exe on MS Windows

example.obj: example.cc
	$(CXX) /Foexample.obj -c example.cc $(CXXFLAGS)

all: all_subdirs example.exe Hapy.lib TestMaker.exe

all_subdirs: 

clean: clean_subdirs 
	-erase /F *.obj *.lib core Hapy.lib example.exe TestMaker.exe

clean_subdirs: 

distclean: distclean_subdirs 
	-erase /F *.obj *.lib core Hapy.lib example.exe TestMaker.exe

distclean_subdirs: 

install: install_subdirs example.exe Hapy.lib TestMaker.exe
install_dirs: 

install: TestMaker.exe_install example.exe_install

install_subdirs: 

semiclean: semiclean_subdirs 
	-erase /F Hapy.lib example.exe TestMaker.exe core

semiclean_subdirs: 

