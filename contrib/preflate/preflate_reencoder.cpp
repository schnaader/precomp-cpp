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

#include <functional>
#include "preflate_block_reencoder.h"
#include "preflate_reencoder.h"
#include "preflate_statistical_codec.h"
#include "preflate_token_predictor.h"
#include "preflate_tree_predictor.h"
#include "support/bitstream.h"
#include "support/memstream.h"

bool preflate_reencode(std::vector<unsigned char>& deflate_raw,
                       const std::vector<unsigned char>& preflate_diff,
                       const std::vector<unsigned char>& unpacked_input) {
  PreflateStatisticalDecoder codec(preflate_diff);
  if (!codec.decodeHeader()) {
    return false;
  }
  PreflateParameters params;
  if (!codec.decodeParameters(params)) {
    return false;
  }
  if (!codec.decodeModel()) {
    return false;
  }
  PreflateTokenPredictor tokenPredictor(params, unpacked_input);
  PreflateTreePredictor treePredictor(unpacked_input);

  MemStream mem;
  BitOutputStream bos(mem);

  PreflateBlockReencoder deflater(bos, unpacked_input);
  bool eof = true;
  do {
    PreflateTokenBlock block = tokenPredictor.decodeBlock(&codec);
    if (!treePredictor.decodeBlock(block, &codec)) {
      return false;
    }
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    eof = tokenPredictor.decodeEOF(&codec);

    deflater.writeBlock(block, eof);
  } while (!eof);
  deflater.flush();
  deflate_raw = mem.extractData();
  return true;
}
bool preflate_reencode(OutputStream& os,
                       const std::vector<unsigned char>& preflate_diff,
                       const std::vector<unsigned char>& unpacked_input,
                       std::function<void(void)> block_callback) {
  PreflateStatisticalDecoder codec(preflate_diff);
  if (!codec.decodeHeader()) {
    return false;
  }
  PreflateParameters params;
  if (!codec.decodeParameters(params)) {
    return false;
  }
  if (!codec.decodeModel()) {
    return false;
  }
  PreflateTokenPredictor tokenPredictor(params, unpacked_input);
  PreflateTreePredictor treePredictor(unpacked_input);

  BitOutputStream bos(os);

  PreflateBlockReencoder deflater(bos, unpacked_input);
  bool eof = true;
  do {
    PreflateTokenBlock block = tokenPredictor.decodeBlock(&codec);
    if (!treePredictor.decodeBlock(block, &codec)) {
      return false;
    }
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    eof = tokenPredictor.decodeEOF(&codec);

    deflater.writeBlock(block, eof);
    block_callback();
  } while (!eof);
  uint8_t remaining_bit_count = (8 - bos.bitPos()) & 7;
  if (remaining_bit_count > 0) {
    bool non_zero_bits = codec.decodeValue(1) != 0;
    if (non_zero_bits) {
      bos.put(codec.decodeValue(remaining_bit_count), remaining_bit_count);
    }
  }
  deflater.flush();
  return true;
}
