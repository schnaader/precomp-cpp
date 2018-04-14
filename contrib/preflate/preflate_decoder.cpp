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
                     const std::vector<unsigned char>& deflate_raw) {
  std::vector<PreflateTokenBlock> blocks;
  MemStream decIn(deflate_raw);
  MemStream decUnc;
  BitInputStream decInBits(decIn);
  OutputCacheStream decOutCache(decUnc);
  PreflateBlockDecoder bdec(decInBits, decOutCache);
  if (bdec.status() != PreflateBlockDecoder::OK) {
    return false;
  }
  bool last;
  unsigned i = 0;
  do {
    PreflateTokenBlock newBlock;
    bool ok = bdec.readBlock(newBlock, last);
    if (!ok) {
      return false;
    }
    blocks.push_back(newBlock);
    ++i;
  } while (!last);
  decOutCache.flush();

  unpacked_output = decUnc.extractData();

  PreflateParameters params = estimatePreflateParameters(unpacked_output, blocks);
  PreflateStatisticalModel model;
  memset(&model, 0, sizeof(model));
  PreflateTokenPredictor tokenPredictor(params, unpacked_output);
  PreflateTreePredictor treePredictor(unpacked_output);
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictor.analyzeBlock(i, blocks[i]);
    treePredictor.analyzeBlock(i, blocks[i]);
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenPredictor.updateModel(&model, i);
    treePredictor.updateModel(&model, i);
  }
  PreflateStatisticalEncoder codec(model);
  codec.encodeHeader();
  codec.encodeParameters(params);
  codec.encodeModel();
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictor.encodeBlock(&codec, i);
    treePredictor.encodeBlock(&codec, i);
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenPredictor.encodeEOF(&codec, i, i + 1 == blocks.size());
  }
  preflate_diff = codec.encodeFinish();
  return true;
}

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
  PreflateStatisticalModel model;
  memset(&model, 0, sizeof(model));
  PreflateTokenPredictor tokenPredictor(params, unpacked_output);
  PreflateTreePredictor treePredictor(unpacked_output);
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictor.analyzeBlock(i, blocks[i]);
    treePredictor.analyzeBlock(i, blocks[i]);
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenPredictor.updateModel(&model, i);
    treePredictor.updateModel(&model, i);
    block_callback();
  }
  PreflateStatisticalEncoder codec(model);
  codec.encodeHeader();
  codec.encodeParameters(params);
  codec.encodeModel();
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictor.encodeBlock(&codec, i);
    treePredictor.encodeBlock(&codec, i);
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenPredictor.encodeEOF(&codec, i, i + 1 == blocks.size());
  }
  if (remaining_bit_count > 0) {
    if (remaining_bits != 0) {
      codec.encodeValue(1, 1);
      codec.encodeValue(remaining_bits, remaining_bit_count);
    } else {
      codec.encodeValue(0, 1);
    }
  }
  preflate_diff = codec.encodeFinish();
  return true;
}
