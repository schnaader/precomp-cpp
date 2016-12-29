// LZMA compression routines, based on the public domain file 04_compress_easy_mt.c from XZ Utils examples
// Original author: Lasse Collin

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "api/lzma.h"

bool check(lzma_ret ret) {
	if (ret == LZMA_OK)
		return true;

	const char *msg;
	switch (ret) {
	case LZMA_MEM_ERROR:
		msg = "Memory allocation failed";
		break;

	case LZMA_OPTIONS_ERROR:
		// We are no longer using a plain preset so this error
		// message has been edited accordingly compared to
		// 01_compress_easy.c.
		msg = "Specified filter chain is not supported";
		break;

	case LZMA_UNSUPPORTED_CHECK:
		msg = "Specified integrity check is not supported";
		break;

	default:
		msg = "Unknown error, possibly a bug";
		break;
	}

	fprintf(stderr, "Error initializing the encoder: %s (error code %u)\n",
			msg, ret);
	return false;
}

#ifdef BIT64
const uint32_t dict = 4;
#else
const uint32_t dict = 2;
#endif

bool init_lzma1(lzma_stream *strm)
{
    lzma_options_lzma opt_lzma;
	if (lzma_lzma_preset(&opt_lzma, UINT32_C(9))) { // LZMA_PRESET_DEFAULT
		fprintf(stderr, "Unsupported preset, possibly a bug\n");
		return false;
	}
    opt_lzma.dict_size *= dict;
    //opt_lzma.mf = LZMA_MF_HC4;
    //opt_lzma.mode = LZMA_MODE_FAST;
    return check(lzma_alone_encoder(strm, &opt_lzma));
}

bool init_lzma2(lzma_stream *strm) {
	lzma_options_lzma opt_lzma;
	if (lzma_lzma_preset(&opt_lzma, UINT32_C(9))) { // LZMA_PRESET_DEFAULT
		fprintf(stderr, "Unsupported preset, possibly a bug\n");
		return false;
	}
	opt_lzma.dict_size *= dict;
	lzma_filter filters[] = {
		{ LZMA_FILTER_LZMA2, &opt_lzma },
		{ LZMA_VLI_UNKNOWN, NULL },
	};
	return check(lzma_stream_encoder(strm, filters, LZMA_CHECK_CRC32));
}

bool init_decoder(lzma_stream *strm)
{
	lzma_ret ret = lzma_auto_decoder(
		strm, UINT64_MAX, 0);
	return check(ret);
}

bool init_encoder_mt(lzma_stream *strm, int threads, uint64_t max_memory, uint64_t &memory_usage)
{
	// The threaded encoder takes the options as pointer to
	// a lzma_mt structure.
	lzma_mt mt;
    
	// No flags are needed.
	mt	.flags = 0;

	// Let liblzma determine a sane block size.
	mt	.block_size = 0;

	// Use no timeout for lzma_code() calls by setting timeout
	// to zero. That is, sometimes lzma_code() might block for
	// a long time (from several seconds to even minutes).
	// If this is not OK, for example due to progress indicator
	// needing updates, specify a timeout in milliseconds here.
	// See the documentation of lzma_mt in lzma/container.h for
	// information how to choose a reasonable timeout.
	mt	.timeout = 110;

	// To use a preset, filters must be set to NULL.
	mt.filters = NULL;

	mt.check = LZMA_CHECK_CRC32;

	mt.threads = threads;

    uint64_t preset_memory_usage;
    int preset_to_use = 0;
    for (int preset = 1; preset <= 9; preset++) {
        mt.preset = preset;
        preset_memory_usage = lzma_stream_encoder_mt_memusage(&mt);
        if (preset_memory_usage > max_memory) break;
        memory_usage = preset_memory_usage;
        preset_to_use = preset;
    }
    
    if (preset_to_use == 0) return false;
    
    mt.preset = preset_to_use;
    
	// Initialize the threaded encoder.
	lzma_ret ret = lzma_stream_encoder_mt(strm, &mt);

    memory_usage = lzma_stream_encoder_mt_memusage(&mt);    
    
    return check(ret);
}