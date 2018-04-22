/* Copyright 2018 Dirk Steinke

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include <string.h>
#include <functional>
#include "preflate_block_decoder.h"
#include "preflate_decoder.h"
#include "preflate_parameter_estimator.h"
#include "preflate_statistical_model.h"
#include "preflate_token_predictor.h"
#include "preflate_tree_predictor.h"
#include "preflate_unpack.h"
#include "support/bitstream.h"
#include "support/memstream.h"
#include "support/outputcachestream.h"

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     uint64_t& deflate_size,
                     InputStream& deflate_raw,
                     std::function<void(void)> block_callback) {
  deflate_size = 0;
  uint64_t deflate_bits = 0;
  size_t prevBitPos = 0;
  BitInputStream decInBits(deflate_raw);
  MemStream decUnc;
  OutputCacheStream decOutCache(decUnc);
  PreflateBlockDecoder bdec(decInBits, decOutCache);
  if (bdec.status() != PreflateBlockDecoder::OK) {
    return false;
  }
  bool last;
  unsigned i = 0;
  std::vector<PreflateTokenBlock> blocks;
  do {
    PreflateTokenBlock newBlock;

    bool ok = bdec.readBlock(newBlock, last);
    if (!ok) {
      return false;
    }
    blocks.push_back(newBlock);
    ++i;
    if (decOutCache.cacheSize() >= 512 * 1024) {
      decOutCache.flushUpTo(decOutCache.cacheEndPos() - (32 * 1024));
    }
    deflate_bits += decInBits.bitPos() - prevBitPos;
    prevBitPos = decInBits.bitPos();
    block_callback();
  } while (!last);
  decOutCache.flush();
  unpacked_output = decUnc.extractData();
  deflate_size = (deflate_bits + 7) >> 3;
  uint8_t remaining_bit_count = (8 - deflate_bits) & 7;
  uint8_t remaining_bits = decInBits.get(remaining_bit_count);

  PreflateParameters params = estimatePreflateParameters(unpacked_output, blocks);
  PreflateStatisticsCounter counter;
  memset(&counter, 0, sizeof(counter));
  PreflateTokenPredictor tokenPredictor(params, unpacked_output);
  PreflateTreePredictor treePredictor(unpacked_output);
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictor.analyzeBlock(i, blocks[i]);
    treePredictor.analyzeBlock(i, blocks[i]);
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenPredictor.updateCounters(&counter, i);
    treePredictor.updateCounters(&counter, i);
    block_callback();
  }
  counter.block.incNonZeroPadding(remaining_bits != 0);
  PreflateMetaEncoder encoder;
  PreflatePredictionEncoder pcodec;
  unsigned modelId = encoder.addModel(counter, params);
  if (!encoder.beginMetaBlockWithModel(pcodec, modelId)) {
    return false;
  }
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictor.encodeBlock(&pcodec, i);
    treePredictor.encodeBlock(&pcodec, i);
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenPredictor.encodeEOF(&pcodec, i, i + 1 == blocks.size());
  }
  pcodec.encodeNonZeroPadding(remaining_bits != 0);
  if (remaining_bits != 0) {
    unsigned bitsToSave = bitLength(remaining_bit_count);
    pcodec.encodeValue(bitsToSave, 3);
    if (bitsToSave > 1) {
      pcodec.encodeValue(remaining_bits, bitsToSave - 1);
    }
  }
  if (!encoder.endMetaBlock(pcodec, unpacked_output.size())) {
    return false;
  }
  preflate_diff = encoder.finish();
  return !encoder.error();
}

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     const std::vector<unsigned char>& deflate_raw) {
  MemStream mem(deflate_raw);
  uint64_t raw_size;
  return preflate_decode(unpacked_output, preflate_diff,
                         raw_size, mem, [] {}) && raw_size == deflate_raw.size();
}
