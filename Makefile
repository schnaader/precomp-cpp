PROGNAME      = precomp
BZIP2_OBJ     = contrib/bzip2/blocksort.o contrib/bzip2/compress.o contrib/bzip2/decompress.o contrib/bzip2/randtable.o contrib/bzip2/bzlib.o contrib/bzip2/crctable.o contrib/bzip2/huffman.o
GIFLIB_OBJ    = contrib/giflib/dgif_lib_gcc.o contrib/giflib/egif_lib_gcc.o contrib/giflib/gifalloc.o contrib/giflib/gif_err.o
PACKJPG_OBJ   = contrib/packjpg/aricoder.o contrib/packjpg/bitops.o contrib/packjpg/packjpg.o
PACKMP3_OBJ   = contrib/packmp3/huffmp3.o contrib/packmp3/packmp3.o
ZLIB_OBJ      = contrib/zlib/adler32.o contrib/zlib/crc32.o contrib/zlib/zutil.o contrib/zlib/trees.o contrib/zlib/inftrees.o contrib/zlib/inffast.o contrib/zlib/inflate.o contrib/zlib/deflate.o
LIBLZMA_OBJ   = contrib/liblzma/alone_decoder.o contrib/liblzma/alone_encoder.o contrib/liblzma/arm.o contrib/liblzma/armthumb.o contrib/liblzma/auto_decoder.o contrib/liblzma/block_buffer_decoder.o contrib/liblzma/block_buffer_encoder.o contrib/liblzma/block_decoder.o contrib/liblzma/block_encoder.o contrib/liblzma/block_header_decoder.o contrib/liblzma/block_header_encoder.o contrib/liblzma/block_util.o contrib/liblzma/check.o contrib/liblzma/common.o contrib/liblzma/crc32_table.o contrib/liblzma/crc32_fast.o contrib/liblzma/crc64_table.o contrib/liblzma/crc64_fast.o contrib/liblzma/delta_common.o contrib/liblzma/delta_decoder.o contrib/liblzma/delta_encoder.o contrib/liblzma/easy_buffer_encoder.o contrib/liblzma/easy_decoder_memusage.o contrib/liblzma/easy_encoder.o contrib/liblzma/easy_encoder_memusage.o contrib/liblzma/easy_preset.o contrib/liblzma/fastpos_table.o contrib/liblzma/filter_buffer_decoder.o contrib/liblzma/filter_buffer_encoder.o contrib/liblzma/filter_common.o contrib/liblzma/filter_decoder.o contrib/liblzma/filter_encoder.o contrib/liblzma/filter_flags_decoder.o contrib/liblzma/filter_flags_encoder.o contrib/liblzma/hardware_cputhreads.o contrib/liblzma/hardware_physmem.o contrib/liblzma/ia64.o contrib/liblzma/index.o contrib/liblzma/index_decoder.o contrib/liblzma/index_encoder.o contrib/liblzma/index_hash.o contrib/liblzma/lzma2_decoder.o contrib/liblzma/lzma2_encoder.o contrib/liblzma/lzma_decoder.o contrib/liblzma/lzma_encoder.o contrib/liblzma/lzma_encoder_optimum_fast.o contrib/liblzma/lzma_encoder_optimum_normal.o contrib/liblzma/lzma_encoder_presets.o contrib/liblzma/lz_decoder.o contrib/liblzma/lz_encoder.o contrib/liblzma/lz_encoder_mf.o contrib/liblzma/outqueue.o contrib/liblzma/powerpc.o contrib/liblzma/price_table.o contrib/liblzma/sha256.o contrib/liblzma/simple_coder.o contrib/liblzma/simple_decoder.o contrib/liblzma/simple_encoder.o contrib/liblzma/sparc.o contrib/liblzma/stream_decoder.o contrib/liblzma/stream_buffer_encoder.o contrib/liblzma/stream_buffer_decoder.o contrib/liblzma/stream_encoder.o contrib/liblzma/stream_flags_decoder.o contrib/liblzma/stream_encoder_mt.o contrib/liblzma/stream_flags_common.o contrib/liblzma/stream_flags_encoder.o contrib/liblzma/tuklib_cpucores.o contrib/liblzma/tuklib_physmem.o contrib/liblzma/vli_decoder.o contrib/liblzma/vli_encoder.o contrib/liblzma/vli_size.o contrib/liblzma/x86.o
LIBLZMA_CPP   = contrib/liblzma/compress_easy_mt.cpp
CFLAGS        = -std=c++11 -DUNIX -DBIT64 -D_FILE_OFFSET_BITS=64 -m64 -O2 -Wall -pthread

.PHONY: all
all: contrib $(PROGNAME)

.PHONY: clean
clean:
	make -C contrib/bzip2 clean
	make -C contrib/giflib clean
	make -C contrib/packjpg clean
	make -C contrib/packmp3 clean
	make -C contrib/zlib clean
	make -C contrib/liblzma clean
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

liblzma:
	make -C contrib/liblzma

contrib: bzip2 giflib packjpg packmp3 zlib liblzma

$(PROGNAME): contrib
	g++ $(CFLAGS) $(GIFLIB_OBJ) $(PACKJPG_OBJ) $(PACKMP3_OBJ) $(BZIP2_OBJ) $(ZLIB_OBJ) $(LIBLZMA_OBJ) $(LIBLZMA_CPP) precomp.cpp -s -oprecomp
