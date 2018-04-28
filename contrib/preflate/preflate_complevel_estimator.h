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

#ifndef PREFLATE_COMPLEVEL_ESTIMATOR_H
#define PREFLATE_COMPLEVEL_ESTIMATOR_H

#include "preflate_predictor_state.h"
#include "preflate_token.h"

struct PreflateCompLevelInfo {
  unsigned possibleCompressionLevels;
  unsigned recommendedCompressionLevel;
  bool zlibCompatible;

  unsigned referenceCount;
  unsigned unfoundReferences;
  unsigned short maxChainDepth;
  unsigned short longestLen3Dist;
  unsigned short longestDistAtHop0;
  unsigned short longestDistAtHop1Plus;
  bool matchToStart;
  bool veryFarMatches;
  bool farLen3Matches;
};

struct PreflateCompLevelEstimatorState {
  PreflateHashChainExt slowHash;
  PreflateHashChainExt fastL1Hash;
  PreflateHashChainExt fastL2Hash;
  PreflateHashChainExt fastL3Hash;
  const std::vector<PreflateTokenBlock>& blocks;
  PreflateCompLevelInfo info;
  uint16_t wsize;

  PreflateCompLevelEstimatorState(const int wbits, const int mbits,
                                  const std::vector<unsigned char>& unpacked_output,
                                  const std::vector<PreflateTokenBlock>& blocks);
  void updateHash(const unsigned len);
  void updateOrSkipHash(const unsigned len);
  void checkMatch(const PreflateToken& token);
  void checkDump(bool early_out);
  void recommend();

private:
  void updateOrSkipSingleFastHash(PreflateHashChainExt&, const unsigned len, const PreflateParserConfig&);
  bool checkMatchSingleFastHash(const PreflateToken& token, const PreflateHashChainExt&, const PreflateParserConfig&,
                                const unsigned hashHead);
  uint16_t matchDepth(const unsigned hashHead, const PreflateToken& targetReference,
                      const PreflateHashChainExt& hash);
  unsigned windowSize() const {
    return wsize;
  }
};

PreflateCompLevelInfo estimatePreflateCompLevel(
    const int wbits, 
    const int mbits,
    const std::vector<unsigned char>& unpacked_output,
    const std::vector<PreflateTokenBlock>& blocks,
    const bool early_out);

#endif /* PREFLATE_COMPLEVEL_ESTIMATOR_H */
