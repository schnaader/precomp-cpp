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

bool init_decoder(lzma_stream *strm)
{
  lzma_ret ret = lzma_auto_decoder(
    strm, UINT64_MAX, 0);
  return check(ret);
}

lzma_filter get_filter(lzma_vli filter_id) {
  lzma_filter result;
  result.id = filter_id;
  result.options = NULL;
  return result;
}

bool init_encoder_mt(lzma_stream *strm, int threads, uint64_t max_memory, uint64_t &memory_usage, uint64_t &block_size)
{
  // The threaded encoder takes the options as pointer to
  // a lzma_mt structure.
  lzma_mt mt;
    
  // No flags are needed.
  mt.flags = 0;

  // Let liblzma determine a sane block size.
  mt.block_size = 0;

  // Use no timeout for lzma_code() calls by setting timeout
  // to zero. That is, sometimes lzma_code() might block for
  // a long time (from several seconds to even minutes).
  // If this is not OK, for example due to progress indicator
  // needing updates, specify a timeout in milliseconds here.
  // See the documentation of lzma_mt in lzma/container.h for
  // information how to choose a reasonable timeout.
  mt.timeout = 110;
  mt.check = LZMA_CHECK_CRC32;

  mt.threads = threads;

  uint64_t preset_memory_usage;
  int preset_to_use = 1; // Always use at least preset 1, even if it exceeds the given memory limit
  for (int preset = 2; preset <= 9; preset++) {
    lzma_options_lzma opt_lzma2;
    if (lzma_lzma_preset(&opt_lzma2, preset)) {
      fprintf(stderr, "Unsupported preset, possibly a bug\n");
      return false;
    }

    lzma_filter filterLzma2;
    filterLzma2.id = LZMA_FILTER_LZMA2;
    filterLzma2.options = &opt_lzma2;

    // the executable filters we use don't change memory usage, so we just ignore them here
    // and set them below
    const lzma_filter filters[] = {
      filterLzma2,
      get_filter(LZMA_VLI_UNKNOWN)
    };

    mt.filters = filters;

    preset_memory_usage = lzma_stream_encoder_mt_memusage(&mt, &block_size);
    if (preset_memory_usage > max_memory) break;
    memory_usage = preset_memory_usage;
    preset_to_use = preset;
  }

  lzma_options_lzma opt_lzma2;
  if (lzma_lzma_preset(&opt_lzma2, preset_to_use)) {
    fprintf(stderr, "Unsupported preset, possibly a bug\n");
    return false;
  }

  lzma_filter filterLzma2;
  filterLzma2.id = LZMA_FILTER_LZMA2;
  filterLzma2.options = &opt_lzma2;

  const lzma_filter filters[] = {
    get_filter(LZMA_FILTER_X86),
    get_filter(LZMA_FILTER_POWERPC),
    //get_filter(LZMA_FILTER_IA64),
    get_filter(LZMA_FILTER_ARM),
    //get_filter(LZMA_FILTER_ARMTHUMB),
    //get_filter(LZMA_FILTER_SPARC),
    filterLzma2,
    get_filter(LZMA_VLI_UNKNOWN)
  };

  mt.filters = filters;

  // Initialize the threaded encoder.
  lzma_ret ret = lzma_stream_encoder_mt(strm, &mt);

  // Determine memory usage and block size
  memory_usage = lzma_stream_encoder_mt_memusage(&mt, &block_size);
    
  return check(ret);
}