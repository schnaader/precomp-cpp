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

#include <algorithm>
#include "preflate_complevel_estimator.h"
#include "preflate_constants.h"
#include "preflate_info.h"
#include "preflate_parameter_estimator.h"
#include "preflate_token_predictor.h"
#include "support/bit_helper.h"

unsigned char estimatePreflateMemLevel(const unsigned maxBlockSize_) {
  unsigned maxBlockSize = maxBlockSize_;
  unsigned mbits = 0;
  while (maxBlockSize > 0) {
    ++mbits; maxBlockSize >>= 1;
  }
  mbits = std::min(std::max(mbits, 7u), 15u);
  return mbits - 6;
}

unsigned char estimatePreflateWindowBits(const unsigned maxDist_) {
  unsigned maxDist = maxDist_;
  maxDist += PreflateConstants::MIN_LOOKAHEAD;
  unsigned wbits = bitLength(maxDist - 1);
  wbits = std::min(std::max(wbits, 9u), 15u);
  return wbits;
}

PreflateStrategy estimatePreflateStrategy(const PreflateStreamInfo& info) {
  if (info.countStoredBlocks == info.countBlocks) {
    return PREFLATE_STORE;
  }
  if (info.countHuffBlocks == info.countBlocks) {
    return PREFLATE_HUFF_ONLY;
  }
  if (info.countRLEBlocks == info.countBlocks) {
    return PREFLATE_RLE_ONLY;
  }
  return PREFLATE_DEFAULT;
}

PreflateHuffStrategy estimatePreflateHuffStrategy(const PreflateStreamInfo& info) {
  if (info.countStaticHuffTreeBlocks == info.countBlocks) {
    return PREFLATE_HUFF_STATIC;
  }
  if (info.countStaticHuffTreeBlocks == 0) {
    return PREFLATE_HUFF_DYNAMIC;
  }
  return PREFLATE_HUFF_MIXED;
}

PreflateParameters estimatePreflateParameters(const std::vector<unsigned char>& unpacked_output,
                                              const size_t off0,
                                              const std::vector<PreflateTokenBlock>& blocks) {
  PreflateStreamInfo info = extractPreflateInfo(blocks);

  PreflateParameters result;
  result.windowBits   = estimatePreflateWindowBits(info.maxDist);
  result.memLevel     = estimatePreflateMemLevel(info.maxTokensPerBlock);
  result.strategy     = estimatePreflateStrategy(info);
  result.huffStrategy = estimatePreflateHuffStrategy(info);
  PreflateCompLevelInfo cl = estimatePreflateCompLevel(result.windowBits, result.memLevel, unpacked_output, off0, blocks, false);
  result.compLevel    = cl.recommendedCompressionLevel;
  result.zlibCompatible = cl.zlibCompatible;
  result.farLen3MatchesDetected = cl.farLen3Matches;
  result.veryFarMatchesDetected = cl.veryFarMatches;
  result.matchesToStartDetected = cl.matchToStart;
  result.log2OfMaxChainDepthM1 = cl.maxChainDepth == 0 ? 0 : bitLength(cl.maxChainDepth - 1);
  return result;
}
