#ifndef PRECOMP_XZ_H
#define PRECOMP_XZ_H

#include "api/lzma.h"

struct lzma_init_mt_extra_parameters {
  bool enable_filter_x86;
  bool enable_filter_powerpc;
  bool enable_filter_ia64;
  bool enable_filter_arm;
  bool enable_filter_armthumb;
  bool enable_filter_sparc;
  bool enable_filter_delta;
  
  int filter_delta_distance;

  int lc, lp, pb;
};

bool init_encoder_mt(lzma_stream *strm, int threads, uint64_t max_memory, 
                     uint64_t &memory_usage, uint64_t &block_size, 
                     const lzma_init_mt_extra_parameters& extra_params);

bool init_decoder(lzma_stream *strm);

#endif /* ifndef PRECOMP_XZ_H */
