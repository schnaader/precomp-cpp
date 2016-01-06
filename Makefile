PROGNAME      = precomp
SOURCE        = gifalloc.c gif_err.c dgif_lib_gcc.c egif_lib_gcc.c bzlib.c blocksort.c crctable.c compress.c decompress.c huffman.c randtable.c
OBJECTS       = adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o
CFLAGS        = -DLINUX -D_FILE_OFFSET_BITS=64 -O2 -Wall
SRC_CONTRIB   = contrib/packjpg/aricoder.cpp contrib/packjpg/bitops.cpp contrib/packjpg/packjpg.cpp
OBJ_CONTRIB   = aricoder.o bitops.o packjpg.o
FLAGS_CONTRIB = -O3 -DUNIX -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer

.PHONY: all
all: contrib $(PROGNAME)

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(PROGNAME)

$(PROGNAME): $(OBJECTS) $(OBJ_CONTRIB)
	g++ $(CFLAGS) $(SOURCE) $(OBJECTS) $(OBJ_CONTRIB) precomp.cpp -s -oprecomp

contrib: $(SRC_CONTRIB)
	g++ -c $(FLAGS_CONTRIB) $(SRC_CONTRIB)

%.o: %.c
	gcc -g -c $(CFLAGS) $<
