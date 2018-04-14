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

#ifndef PREFLATE_PARAMETER_ESTIMATOR_H
#define PREFLATE_PARAMETER_ESTIMATOR_H

/* deflate has four parameters:
* - strategy: the strategy can usually be guessed by looking on the given deflate stream
*             (e.g. only stored blocks -> stored,
*                   max distance = 0 -> huffman-only,
*                   max distance = 1 -> rle,
*                   only fixed huffman trees -> fixed-huffman-tree,
*                   otherwise default)
* - window bits: known by max distance, less window bits would be impossible, more window
*                bits would be pointless
* - mem level: used for hash calculation and number of tokens per block
*              the latter can be used to put a lower limit on mem level
* - compression level: parameters for the reference finder
*
* When reencoding a deflate stream, the predictor has to make a token proposal (either to
* encode a literal or a (dist, len) pair. A correction data stream will either accept the
* proposal, or change it to the correct values. The corrected values are then fed to the
* deflate encoder, and to the predictor.
*
* The main problem is to find the missing deflate parameters (compression level and
* mem level) to minimize the number and complexity of required corrections.
* Data streams that were encoded with zlib should get perfect recognition,
* requiring only the detected deflate parameters to be encoded for perfect reconstruction.
* Data streams from other encoders (7zip, kzip, ...) should be reconstructible with minimal
* corrective instructions, similar to reflate.
*
* kzip does not limit block size to < 64k tokens, while zlib enforces it for various reasons
* (and defaults to max 16k tokens).
* Prediction for end-of-block is therefore independent of literal/reference prediction.
*
* Mixing or interpolating the prediction from different parameter packs is
* possible, but not planned right now.
*/

#include "preflate_info.h"
#include "preflate_parser_config.h"
#include "preflate_token.h"

enum PreflateStrategy {
  PREFLATE_DEFAULT,
  PREFLATE_RLE_ONLY,
  PREFLATE_HUFF_ONLY,
  PREFLATE_STORE
};
enum PreflateHuffStrategy {
  PREFLATE_HUFF_DYNAMIC,
  PREFLATE_HUFF_MIXED,
  PREFLATE_HUFF_STATIC,
};

struct PreflateParameters {
  PreflateStrategy strategy;
  PreflateHuffStrategy huffStrategy;
  bool zlibCompatible;
  unsigned char windowBits;
  unsigned char memLevel;
  unsigned char compLevel;
  // true if matches of len 3 with a distance > 4096 are allowed
  // (disallowed by zlib level 4+)
  bool farLen3MatchesDetected; 
  // true if matches of distance >= 32768 - (MAX_MATCH + MIN_MATCH + 1) are allowed
  // or > 32768 - (MAX_MATCH + MIN_MATCH + 1) if it's the first node in the hash chain
  // (disallowed by zlib)
  bool veryFarMatchesDetected;
  // true if matches to start of stream are allowed
  // (disallowed by zlib)
  bool matchesToStartDetected;
  // log2 of maximal found chain depth - 1
  // so, 9 to 16 have value 3
  unsigned char log2OfMaxChainDepthM1;


  bool isFastCompressor() const {
    return compLevel >= 1 && compLevel <= 3;
  }
  bool isSlowCompressor() const {
    return compLevel >= 4 && compLevel <= 9;
  }
  const PreflateParserConfig& config() const {
    return isFastCompressor() ? fastPreflateParserSettings[compLevel - 1]
      : slowPreflateParserSettings[isSlowCompressor() ? compLevel - 4 : 5];
  }
};

unsigned char estimatePreflateMemLevel(const unsigned maxBlockSize);
PreflateStrategy estimatePreflateStrategy(const PreflateStreamInfo&);
PreflateHuffStrategy estimatePreflateHuffStrategy(const PreflateStreamInfo&);
unsigned char estimatePreflateWindowBits(const unsigned maxDist);

PreflateParameters estimatePreflateParameters(const std::vector<unsigned char>& unpacked_output,
                                              const std::vector<PreflateTokenBlock>& blocks);

#endif /* PREFLATE_PARAMETER_ESTIMATOR_H */
