#
# This is a generated file. Modifications are futile.
#

.SUFFIXES:
.SUFFIXES: .h .cc .obj .lib

STD_INCLUDE = /I"\Program Files\Microsoft Visual Studio\VC98\INCLUDE"
LCL_INCLUDE = /I..
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

AnyToString.cc: xstd.h iomanip.h AnyToString.h

AnyToString.obj: AnyToString.cc
	$(CXX) /FoAnyToString.obj -c AnyToString.cc $(CXXFLAGS)

Assert.cc: xstd.h Assert.h

Assert.obj: Assert.cc
	$(CXX) /FoAssert.obj -c Assert.cc $(CXXFLAGS)

Base64Encoding.cc: xstd.h Base64Encoding.h

Base64Encoding.obj: Base64Encoding.cc
	$(CXX) /FoBase64Encoding.obj -c Base64Encoding.cc $(CXXFLAGS)

Checksum.cc: xstd.h Assert.h Checksum.h

Checksum.h: Size.h

Checksum.obj: Checksum.cc
	$(CXX) /FoChecksum.obj -c Checksum.cc $(CXXFLAGS)

Clock.cc: xstd.h Clock.h

Clock.h: Time.h

Clock.obj: Clock.cc
	$(CXX) /FoClock.obj -c Clock.cc $(CXXFLAGS)

Error.cc: xstd.h Error.h

Error.obj: Error.cc
	$(CXX) /FoError.obj -c Error.cc $(CXXFLAGS)

FuzzyTime.cc: xstd.h FuzzyTime.h

FuzzyTime.h: Time.h Math.h

FuzzyTime.obj: FuzzyTime.cc
	$(CXX) /FoFuzzyTime.obj -c FuzzyTime.cc $(CXXFLAGS)

Math.cc: xstd.h Math.h

Math.obj: Math.cc
	$(CXX) /FoMath.obj -c Math.cc $(CXXFLAGS)

NetAddr.cc: xstd.h Assert.h NetAddr.h

NetAddr.h: xstd.h

NetAddr.obj: NetAddr.cc
	$(CXX) /FoNetAddr.obj -c NetAddr.cc $(CXXFLAGS)

Select.cc: xstd.h Assert.h Math.h Clock.h Select.h

Select.h: Time.h

Select.obj: Select.cc
	$(CXX) /FoSelect.obj -c Select.cc $(CXXFLAGS)

Size.cc: xstd.h Size.h

Size.obj: Size.cc
	$(CXX) /FoSize.obj -c Size.cc $(CXXFLAGS)

Socket.cc: xstd.h Assert.h Socket.h

Socket.h: Time.h Size.h Error.h NetAddr.h

Socket.obj: Socket.cc
	$(CXX) /FoSocket.obj -c Socket.cc $(CXXFLAGS)

StringToAny.cc: xstd.h Assert.h Bind2Ref.h StringToAny.h

StringToAny.h: Time.h NetAddr.h

StringToAny.obj: StringToAny.cc
	$(CXX) /FoStringToAny.obj -c StringToAny.cc $(CXXFLAGS)

Time.cc: xstd.h Assert.h Time.h

Time.obj: Time.cc
	$(CXX) /FoTime.obj -c Time.cc $(CXXFLAGS)

UniqueNum.cc: xstd.h Time.h UniqueNum.h

UniqueNum.obj: UniqueNum.cc
	$(CXX) /FoUniqueNum.obj -c UniqueNum.cc $(CXXFLAGS)

iomanip.h: iosfwd.h

iostream.h: iosfwd.h

sstream.h: xstd.h

string.h: xstd.h

xstd.lib: Assert.obj Base64Encoding.obj Clock.obj Checksum.obj Error.obj Math.obj NetAddr.obj Select.obj Size.obj Socket.obj UniqueNum.obj StringToAny.obj AnyToString.obj Time.obj FuzzyTime.obj
	-erase /F $@
	$(AR_R) $(LDFLAGS) /out:$@ \
	Assert.obj Base64Encoding.obj Clock.obj Checksum.obj Error.obj Math.obj NetAddr.obj Select.obj Size.obj Socket.obj UniqueNum.obj StringToAny.obj AnyToString.obj Time.obj FuzzyTime.obj

all: all_subdirs xstd.lib

all_subdirs: 

clean: clean_subdirs 
	-erase /F *.obj *.lib core xstd.lib

clean_subdirs: 

distclean: distclean_subdirs 
	-erase /F *.obj *.lib core xstd.lib

distclean_subdirs: 

install: install_subdirs xstd.lib

install_subdirs: 

semiclean: semiclean_subdirs 
	-erase /F xstd.lib core

semiclean_subdirs: 

