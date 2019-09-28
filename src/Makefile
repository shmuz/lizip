# Makefile

PROJECT = lizip
OBJ = lzip.o lunzip.o common.o
MYCFLAGS = -DWINDLL
MYINCS = -I$(PATH_INFOZIP)
MYLIBS = zip32z64.a unzip32.a
lzip.o: MYCFLAGS += -DGLOBALS

$(PROJECT).dll : $(MYLIBS)

zip32z64.a : zip32z64.def
	dlltool --dllname zip32z64.dll --def $< --output-lib $@ -k

unzip32.a : unzip32.def
	dlltool --dllname unzip32.dll --def $< --output-lib $@ -k

common.o lzip.o lunzip.o : common.h

include $(PATH_SYSTEM)\_mingw.mak
