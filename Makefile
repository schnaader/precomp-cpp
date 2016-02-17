PROGNAME      = precomp
BZIP2_OBJ     = contrib/bzip2/blocksort.o contrib/bzip2/compress.o contrib/bzip2/decompress.o contrib/bzip2/randtable.o contrib/bzip2/bzlib.o contrib/bzip2/crctable.o contrib/bzip2/huffman.o
GIFLIB_OBJ    = contrib/giflib/dgif_lib_gcc.o contrib/giflib/egif_lib_gcc.o contrib/giflib/gifalloc.o contrib/giflib/gif_err.o
PACKJPG_OBJ   = contrib/packjpg/aricoder.o contrib/packjpg/bitops.o contrib/packjpg/packjpg.o
PACKMP3_OBJ   = contrib/packmp3/huffmp3.o contrib/packmp3/packmp3.o
ZLIB_OBJ      = contrib/zlib/adler32.o contrib/zlib/crc32.o contrib/zlib/zutil.o contrib/zlib/trees.o contrib/zlib/inftrees.o contrib/zlib/inffast.o contrib/zlib/inflate.o contrib/zlib/deflate.o
CFLAGS        = -DUNIX -D_FILE_OFFSET_BITS=64 -O2 -Wall

.PHONY: all
all: contrib $(PROGNAME)

.PHONY: clean
clean:
	make -C contrib/bzip2 clean
	make -C contrib/giflib clean
	make -C contrib/packjpg clean
	make -C contrib/packmp3 clean
	make -C contrib/zlib clean
	rm -f *.o

bzip2:
	make -C contrib/bzip2

giflib:
	make -C contrib/giflib

packjpg:
	make -C contrib/packjpg
	
packmp3:
	make -C contrib/packmp3

zlib:
	make -C contrib/zlib

contrib: bzip2 giflib packjpg packmp3 zlib

$(PROGNAME): contrib
	g++ $(CFLAGS) $(GIFLIB_OBJ) $(PACKJPG_OBJ) $(PACKMP3_OBJ) $(BZIP2_OBJ) $(ZLIB_OBJ) precomp.cpp -s -oprecomp
