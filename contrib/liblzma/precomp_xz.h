#ifndef PRECOMP_XZ_H
#define PRECOMP_XZ_H

#include "api/lzma.h"

bool init_lzma1(lzma_stream *strm);
bool init_lzma2(lzma_stream *strm);
bool init_encoder_mt(lzma_stream *strm, int threads, uint64_t &memory_usage);
bool init_decoder(lzma_stream *strm);

#endif /* ifndef PRECOMP_XZ_H */
