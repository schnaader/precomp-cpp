PROGNAME      = precomp
CFLAGS        = -DLINUX -D_FILE_OFFSET_BITS=64 -O2 -Wall
SRC_PACKJPG   = contrib/packjpg/aricoder.cpp contrib/packjpg/bitops.cpp contrib/packjpg/packjpg.cpp
SRC_PACKMP3   = contrib/packmp3/huffmp3.cpp contrib/packmp3/packmp3.cpp
FLAGS_PACKJPG = -O3 -DUNIX -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer
OBJ_ZLIB      = contrib/zlib/adler32.o contrib/zlib/crc32.o contrib/zlib/zutil.o contrib/zlib/trees.o contrib/zlib/inftrees.o contrib/zlib/inffast.o contrib/zlib/inflate.o contrib/zlib/deflate.o
OBJ_BZIP2     = contrib/bzip2/bzlib.o contrib/bzip2/blocksort.o contrib/bzip2/crctable.o contrib/bzip2/compress.o contrib/bzip2/decompress.o contrib/bzip2/huffman.o contrib/bzip2/randtable.o
OBJ_GIFLIB    = contrib/giflib/gifalloc.o contrib/giflib/gif_err.o contrib/giflib/dgif_lib_gcc.o contrib/giflib/egif_lib_gcc.o
OBJECTS       = aricoder.o bitops.o packjpg.o huffmp3.o packmp3.o adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o bzlib.o blocksort.o crctable.o compress.o decompress.o huffman.o randtable.o gifalloc.o gif_err.o dgif_lib_gcc.o egif_lib_gcc.o

.PHONY: all
all: packjpg packmp3 $(PROGNAME)

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(PROGNAME)

$(PROGNAME): $(OBJ_ZLIB) $(OBJ_BZIP2) $(OBJ_GIFLIB)
	g++ $(CFLAGS) $(OBJECTS) precomp.cpp -s -oprecomp

packjpg: $(SRC_PACKJPG)
	g++ -c $(FLAGS_PACKJPG) $(SRC_PACKJPG)

packmp3: $(SRC_PACKMP3)
	g++ -c $(FLAGS_PACKJPG) $(SRC_PACKMP3)

%.o: %.c
	gcc -g -c $(CFLAGS) $<
