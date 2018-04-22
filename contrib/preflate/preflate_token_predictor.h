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

#ifndef PREFLATE_TOKEN_PREDICTOR_H
#define PREFLATE_TOKEN_PREDICTOR_H

#include <vector>

#include "preflate_parameter_estimator.h"
#include "preflate_predictor_state.h"
#include "preflate_statistical_codec.h"

struct PreflateStatisticalModel;
struct PreflateStatisticalCodec;

struct PreflateTokenPredictor {
  PreflatePredictorState state;
  PreflateHashChainExt     hash;
  PreflateParameters     params;
  bool predictionFailure;
  bool                  fast;
  unsigned prevLen;
  PreflateToken pendingToken;
  unsigned currentTokenCount;
  bool emptyBlockAtEnd;

  struct BlockAnalysisResult {
    PreflateTokenBlock::Type type;
    unsigned tokenCount;
    bool blockSizePredicted;
    bool inputEOF;
    bool lastBlock;
    uint8_t paddingBits, paddingCounts;
    std::vector<unsigned char> tokenInfo;
    std::vector<signed> correctives;
  };
  std::vector<BlockAnalysisResult> analysisResults;

  PreflateTokenPredictor(const PreflateParameters& params,
                        const std::vector<unsigned char>& dump);
  void analyzeBlock(const unsigned blockno, 
                    const PreflateTokenBlock& block);
  void updateCounters(PreflateStatisticsCounter*,
                   const unsigned blockno);
  void encodeBlock(PreflatePredictionEncoder*,
                   const unsigned blockno);
  void encodeEOF(PreflatePredictionEncoder*,
                 const unsigned blockno,
                 const bool lastBlock);

  PreflateTokenBlock decodeBlock(PreflatePredictionDecoder*);
  bool decodeEOF(PreflatePredictionDecoder*);

  bool predictEOB();
  PreflateToken predictToken();
  bool repredictReference(PreflateToken& token);
  PreflateRematchInfo repredictMatch(const PreflateToken&);
  unsigned recalculateDistance(const PreflateToken&, const unsigned hops);
  void commitToken(const PreflateToken&);
};

#endif /* PREFLATE_TOKEN_PREDICTOR_H */
