# packXXX Makefile, based on UniMake: Universal Makefile
# Created by Matthias Stirner, 01/2016

TARGET   = packARI
CC       = gcc
CPP      = g++
RC       = windres -O coff
CPPFLAGS = -I. -O3 -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer
LDFLAGS  = -static -static-libgcc -static-libstdc++
CSRC     = $(wildcard *.c)
CPPSRC   = $(wildcard *.cpp)
DEPS     = $(wildcard *.h) Makefile
OBJ      = $(patsubst %.c,%.o,$(CSRC)) $(patsubst %.cpp,%.o,$(CPPSRC))

# conditional stuff
ifeq ($(OS),Windows_NT)
LDFLAGS  += -lpthread -L libwinpthread-1.dll
RES       = icons.res
UPX      := -upx --best --lzma $(TARGET).exe
else
CPPFLAGS += -DUNIX
RC        = 
RES       =
UPX       =
endif

%.o: %.cpp $(DEPS)
	$(CPP) -c -o $@ $< $(CPPFLAGS)
	
%.res: %.rc
	@-$(RC) $< $@

$(TARGET): $(OBJ) $(RES)
	$(CPP) -o $@ $^ -s $(LDFLAGS)
	$(UPX)

.PHONY: all dev lib dll

all: $(TARGET)

dev: CPPFLAGS += -DDEV_BUILD
dev: $(TARGET)

lib: CPPFLAGS += -DBUILD_LIB
lib: $(OBJ)
	ar r $(TARGET)lib.a $(OBJ)
	ranlib $(TARGET)lib.a
    
dll: CPPFLAGS += -DBUILD_DLL
dll: LDFLAGS  += -Wl,--out-implib,libpackJPG.a -fvisibility=hidden
dll: $(OBJ)
	$(CPP) -shared -o $(TARGET).dll $^ $(LDFLAGS)

clean:
	@echo clean...
	@-rm *.o *.a $(TARGET) $(TARGET).exe $(TARGET).dll
