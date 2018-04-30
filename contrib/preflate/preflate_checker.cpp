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

#include <stdio.h>
#include <string.h>
#include "preflate_block_decoder.h"
#include "preflate_block_reencoder.h"
#include "preflate_checker.h"
#include "preflate_parameter_estimator.h"
#include "preflate_statistical_model.h"
#include "preflate_token_predictor.h"
#include "preflate_tree_predictor.h"
#include "support/bitstream.h"
#include "support/memstream.h"
#include "support/outputcachestream.h"

#include <algorithm>

bool preflate_checker(const std::vector<unsigned char>& deflate_raw) {
  printf("Checking raw deflate file of size %d\n", (int)deflate_raw.size());

  MemStream decIn(deflate_raw);
  MemStream decUnc;
  BitInputStream decInBits(decIn);
  OutputCacheStream decOutCache(decUnc);
  std::vector<PreflateTokenBlock> blocks;
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
      printf("inflating error (preflate)\n");
      return false;
    }
    blocks.push_back(newBlock);
    ++i;
  } while (!last);
  uint8_t remaining_bit_count = (8 - decInBits.bitPos()) & 7;
  uint8_t remaining_bits = decInBits.get(remaining_bit_count);
  decOutCache.flush();
  std::vector<unsigned char> unpacked_output = decUnc.extractData();
  printf("Unpacked data has size %d\n", (int)unpacked_output.size());

  // Encode
  PreflateParameters paramsE = estimatePreflateParameters(unpacked_output, 0, blocks);
  printf("prediction parameters: w %d, c %d, m %d, zlib %d, farL3M %d, very far M %d, M2S %d, log2CD %d\n",
         paramsE.windowBits, paramsE.compLevel, paramsE.memLevel,
         paramsE.zlibCompatible, paramsE.farLen3MatchesDetected,
         paramsE.veryFarMatchesDetected, paramsE.matchesToStartDetected,
         paramsE.log2OfMaxChainDepthM1);

  PreflateStatisticsCounter counterE;
  memset(&counterE, 0, sizeof(counterE));
  PreflateTokenPredictor tokenPredictorE(paramsE, unpacked_output, 0);
  PreflateTreePredictor treePredictorE(unpacked_output, 0);
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictorE.analyzeBlock(i, blocks[i]);
    if (tokenPredictorE.predictionFailure) {
      printf("block %d: compress failed token prediction\n", i);
      return false;
    }
    treePredictorE.analyzeBlock(i, blocks[i]);
    if (treePredictorE.predictionFailure) {
      printf("block %d: compress failed tree prediction\n", i);
      return false;
    }
    tokenPredictorE.updateCounters(&counterE, i);
    treePredictorE.updateCounters(&counterE, i);
  }
  counterE.block.incNonZeroPadding(remaining_bits != 0);

  counterE.print();

  PreflateMetaEncoder codecE;
  if (codecE.error()) {
    return false;
  }
  PreflatePredictionEncoder pcodecE;
  unsigned modelId = codecE.addModel(counterE, paramsE);
  if (!codecE.beginMetaBlockWithModel(pcodecE, modelId)) {
    return false;
  }
  for (unsigned i = 0, n = blocks.size(); i < n; ++i) {
    tokenPredictorE.encodeBlock(&pcodecE, i);
    if (tokenPredictorE.predictionFailure) {
      printf("block %d: compress failed token encoding\n", i);
      return false;
    }
    treePredictorE.encodeBlock(&pcodecE, i);
    if (treePredictorE.predictionFailure) {
      printf("block %d: compress failed tree encoding\n", i);
      return false;
    }
    tokenPredictorE.encodeEOF(&pcodecE, i, i + 1 == blocks.size());
  }
  pcodecE.encodeNonZeroPadding(remaining_bits != 0);
  if (remaining_bits != 0) {
    unsigned bitsToSave = bitLength(remaining_bits);
    pcodecE.encodeValue(bitsToSave, 3);
    if (bitsToSave > 1) {
      pcodecE.encodeValue(remaining_bits & ((1 << (bitsToSave - 1)) - 1), bitsToSave - 1);
    }
  }
  if (!codecE.endMetaBlock(pcodecE, unpacked_output.size())) {
    return false;
  }
  std::vector<unsigned char> preflate_diff = codecE.finish();
  printf("Prediction diff has size %d\n", (int)preflate_diff.size());

  // Decode
  PreflateMetaDecoder codecD(preflate_diff, unpacked_output.size());
  PreflatePredictionDecoder pcodecD;
  PreflateParameters paramsD;
  if (codecD.error() || codecD.metaBlockCount() != 1) {
    return false;
  }
  if (!codecD.beginMetaBlock(pcodecD, paramsD, 0)) {
    return false;
  }
  if (paramsD.windowBits != paramsE.windowBits) {
    printf("parameter decoding failed: windowBits mismatch\n");
    return false;
  }
  if (paramsD.memLevel != paramsE.memLevel) {
    printf("parameter decoding failed: memLevel mismatch\n");
    return false;
  }
  if (paramsD.compLevel != paramsE.compLevel) {
    printf("parameter decoding failed: compLevel mismatch\n");
    return false;
  }
  if (paramsD.zlibCompatible != paramsE.zlibCompatible) {
    printf("parameter decoding failed: zlib compatible flag mismatch\n");
    return false;
  }
  if (!paramsD.zlibCompatible && (0
//      || paramsD.farLen3MatchesDetected != paramsE.farLen3MatchesDetected
      || paramsD.veryFarMatchesDetected != paramsE.veryFarMatchesDetected
      || paramsD.matchesToStartDetected != paramsE.matchesToStartDetected
      || paramsD.log2OfMaxChainDepthM1 != paramsE.log2OfMaxChainDepthM1)) {
    printf("parameter decoding failed: non-zlib flag mismatch\n");
    return false;
  }

  if (!isEqual(pcodecD, pcodecE)) {
    printf("decoded model differs from original\n");
    return false;
  }

  PreflateTokenPredictor tokenPredictorD(paramsD, unpacked_output, 0);
  PreflateTreePredictor treePredictorD(unpacked_output, 0);

  MemStream mem;
  BitOutputStream bos(mem);

  PreflateBlockReencoder deflater(bos, unpacked_output, 0);
  unsigned blockno = 0;
  bool eof = true;
  do {
    if (blockno >= blocks.size()) {
      printf("block number too big: org %d, new %d\n", (int)blocks.size(), blockno);
      return false;
    }
    PreflateTokenBlock block = tokenPredictorD.decodeBlock(&pcodecD);
    if (tokenPredictorD.predictionFailure) {
      printf("block %d: token uncompress failed\n", blockno);
      return false;
    }
    if (block.type != blocks[blockno].type) {
      printf("block %d: type differs: org %d, new %d\n", blockno, blocks[blockno].type, block.type);
      return false;
    }
    for (unsigned i = 0, n = std::min(block.tokens.size(), blocks[blockno].tokens.size()); i < n; ++i) {
      PreflateToken orgToken = blocks[blockno].tokens[i];
      PreflateToken newToken = block.tokens[i];
      if (newToken.len != orgToken.len || newToken.dist != orgToken.dist) {
        printf("block %d: generated token %d differs: org(%d,%d), new(%d,%d)\n",
               blockno, i, orgToken.len, orgToken.dist, newToken.len, newToken.dist);
        return false;
      }
    }
    if (block.tokens.size() != blocks[blockno].tokens.size()) {
      printf("block %d: differing token count: org %d, new %d\n",
             blockno, (int)blocks[blockno].tokens.size(), (int)block.tokens.size());
      return false;
    }

    if (!treePredictorD.decodeBlock(block, &pcodecD)) {
      printf("block %d: tree uncompress failed\n", blockno);
      return false;
    }
    if (treePredictorD.predictionFailure) {
      printf("block %d: tree uncompress failed\n", blockno);
      return false;
    }
    if (block.type == PreflateTokenBlock::DYNAMIC_HUFF) {
      if (block.nlen != blocks[blockno].nlen) {
        printf("block %d: literal/len count differs: org %d, new %d\n",
               blockno, blocks[blockno].nlen, block.nlen);
        return false;
      }
      if (block.ndist != blocks[blockno].ndist) {
        printf("block %d: dist count differs: org %d, new %d\n",
               blockno, blocks[blockno].ndist, block.ndist);
        return false;
      }
      if (block.ncode != blocks[blockno].ncode) {
        printf("block %d: tree code count differs: org %d, new %d\n",
               blockno, blocks[blockno].ncode, block.ncode);
        return false;
      }
      if (block.treecodes != blocks[blockno].treecodes) {
        printf("block %d: generated tree codes differs\n", blockno);
        return false;
      }
    }
    eof = tokenPredictorD.decodeEOF(&pcodecD);
    deflater.writeBlock(block, eof);
    ++blockno;
  } while (!eof);
  bool non_zero_bits = pcodecD.decodeNonZeroPadding();
  if (non_zero_bits) {
    unsigned bitsToLoad = pcodecD.decodeValue(3);
    unsigned padding = 0;
    if (bitsToLoad > 0) {
      padding = (1 << (bitsToLoad - 1)) + pcodecD.decodeValue(bitsToLoad - 1);
    }
    bos.put(padding, bitsToLoad);
  }
  if (!codecD.endMetaBlock(pcodecD)) {
    return false;
  }
  deflater.flush();

  std::vector<unsigned char> deflate_raw_out = mem.extractData();
  for (unsigned i = 0, n = std::min(deflate_raw.size(), deflate_raw_out.size()); i < n; ++i) {
    if (deflate_raw[i] != deflate_raw_out[i]) {
      printf("created deflate stream differs at offset %d\n", i);
      return false;
    }
  }
  if (deflate_raw.size() != deflate_raw_out.size()) {
    printf("created deflate streams differs in size: org %d, new %d\n", 
           (int)deflate_raw.size(), (int)deflate_raw_out.size());
    return false;
  }
  printf("Success\n");
  return true;
}
