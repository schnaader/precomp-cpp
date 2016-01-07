PROGNAME      = precomp
SOURCE        = contrib/giflib/gifalloc.c contrib/giflib/gif_err.c contrib/giflib/dgif_lib_gcc.c contrib/giflib/egif_lib_gcc.c contrib/bzip2/bzlib.c contrib/bzip2/blocksort.c contrib/bzip2/crctable.c contrib/bzip2/compress.c contrib/bzip2/decompress.c contrib/bzip2/huffman.c contrib/bzip2/randtable.c
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

contrib/zlib/%.o: contrib/zlib/%.c
	gcc -g -c $(CFLAGS) $<
