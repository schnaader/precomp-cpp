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

bool preflate_reencode(OutputStream& os,
                       const std::vector<unsigned char>& preflate_diff,
                       const std::vector<unsigned char>& unpacked_input,
                       std::function<void(void)> block_callback) {
  PreflateMetaDecoder decoder(preflate_diff, unpacked_input);
  if (decoder.error()) {
    return false;
  }
  if (decoder.metaBlockCount() != 1) {
    return false;
  }
  PreflatePredictionDecoder pcodec;
  PreflateParameters params;
  if (!decoder.beginMetaBlock(pcodec, params, 0)) {
    return false;
  }
  PreflateTokenPredictor tokenPredictor(params, unpacked_input);
  PreflateTreePredictor treePredictor(unpacked_input);

  BitOutputStream bos(os);

  PreflateBlockReencoder deflater(bos, unpacked_input);
  bool eof = true;
  do {
    PreflateTokenBlock block = tokenPredictor.decodeBlock(&pcodec);
    if (!treePredictor.decodeBlock(block, &pcodec)) {
      return false;
    }
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    eof = tokenPredictor.decodeEOF(&pcodec);

    deflater.writeBlock(block, eof);
    block_callback();
  } while (!eof);
  bool non_zero_bits = pcodec.decodeValue(1) != 0;
  if (non_zero_bits) {
    unsigned bitsToLoad = pcodec.decodeValue(3);
    unsigned padding = 0;
    if (bitsToLoad > 0) {
      padding = (1 << (bitsToLoad - 1)) + pcodec.decodeValue(bitsToLoad - 1);
    }
    bos.put(padding, bitsToLoad);
  }
  if (!decoder.endMetaBlock(pcodec)) {
    return false;
  }
  deflater.flush();
  return true;
}
bool preflate_reencode(std::vector<unsigned char>& deflate_raw,
                       const std::vector<unsigned char>& preflate_diff,
                       const std::vector<unsigned char>& unpacked_input) {
  MemStream mem;
  bool result = preflate_reencode(mem, preflate_diff, unpacked_input, [] {});
  deflate_raw = mem.extractData();
  return result;
}
