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

#ifndef PREFLATE_TREE_PREDICTOR_H
#define PREFLATE_TREE_PREDICTOR_H

#include <vector>

#include "preflate_constants.h"
#include "preflate_input.h"
#include "preflate_parameter_estimator.h"

struct PreflateStatisticsCounter;
struct PreflatePredictionDecoder;
struct PreflatePredictionEncoder;

enum TreeCodeType {
  TCT_BITS = 0, TCT_REP = 1, TCT_REPZS = 2, TCT_REPZL = 3
};

struct PreflateTreePredictor {
  PreflateInput input;
  bool predictionFailure;

  struct BlockAnalysisResult {
    PreflateTokenBlock::Type blockType;
    std::vector<unsigned char> tokenInfo;
    std::vector<signed> correctives;
  };
  std::vector<BlockAnalysisResult> analysisResults;

  void collectTokenStatistics(
      unsigned LcodeFrequencies[],
      unsigned DcodeFrequencies[],
      unsigned& Lcount,
      unsigned& Dcount,
      const PreflateTokenBlock& block);
  unsigned buildLBitlenghs(
      unsigned char bitLengths[],
      unsigned Lcodes[]);
  unsigned buildDBitlenghs(
      unsigned char bitLengths[],
      unsigned Dcodes[]);
  unsigned buildTCBitlengths(
      unsigned char (&bitLengths)[PreflateConstants::CODETREE_CODE_COUNT],
      unsigned (&BLfreqs)[PreflateConstants::CODETREE_CODE_COUNT]);


  unsigned calcBitLengths(unsigned char* symBitLen,
                          const unsigned* symFreq,
                          const unsigned symCount,
                          const unsigned maxBits,
                          const unsigned minMaxCode);

  TreeCodeType predictCodeType(const unsigned char* symBitLen,
                       const unsigned symCount,
                       const bool first);
  unsigned char predictCodeData(const unsigned char* symBitLen,
                               const TreeCodeType type,
                               const unsigned symCount,
                               const bool first);
  void predictLDTrees(BlockAnalysisResult& analysis,
                      unsigned* frequencies,
                      const unsigned char* symBitLen,
                      const unsigned symLCount,
                      const unsigned symDCount,
                      const unsigned char* targetCodes,
                      const unsigned targetCodeSize);
  unsigned reconstructLDTrees(PreflatePredictionDecoder* codec,
                      unsigned* frequencies,
                      unsigned char* targetCodes,
                      unsigned targetCodeSize,
                      const unsigned char* symBitLen,
                      const unsigned symLCount,
                      const unsigned symDCount);

  PreflateTreePredictor(const std::vector<unsigned char>& dump, const size_t offset);
  void analyzeBlock(const unsigned blockno,
                    const PreflateTokenBlock& block);
  void updateCounters(PreflateStatisticsCounter*,
                   const unsigned blockno);
  void encodeBlock(PreflatePredictionEncoder*,
                   const unsigned blockno);

  bool decodeBlock(PreflateTokenBlock& block, PreflatePredictionDecoder*);
};

#endif /* PREFLATE_TREE_PREDICTOR_H */
