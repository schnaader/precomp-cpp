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
#include "preflate_constants.h"
#include "preflate_statistical_model.h"
#include "preflate_token_predictor.h"
#include "support/bit_helper.h"

PreflateTokenPredictor::PreflateTokenPredictor(
    const PreflateParameters& params_,
    const std::vector<unsigned char>& dump)
  : state(hash, params_.config(), params_.windowBits, params_.memLevel)
  , hash(dump, params_.memLevel)
  , params(params_)
  , predictionFailure(false)
  , fast(params_.isFastCompressor())
  , prevLen(0)
  , pendingToken(PreflateToken::NONE)
  , emptyBlockAtEnd(false) {

  if (state.availableInputSize() >= 2) {
    hash.updateRunningHash(state.inputCursor()[0]);
    hash.updateRunningHash(state.inputCursor()[1]);
  }
}

bool PreflateTokenPredictor::predictEOB() {
  return state.availableInputSize() == 0 || currentTokenCount == state.maxTokenCount;
}
void PreflateTokenPredictor::commitToken(const PreflateToken& t) {
  if (fast && t.len > state.lazyMatchLength()) {
    hash.skipHash(t.len);
  } else {
    hash.updateHash(t.len);
  }
}
#  define TOO_FAR 4096
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

PreflateToken PreflateTokenPredictor::predictToken() {
  if (state.currentInputPos() == 0 || state.availableInputSize() < PreflateConstants::MIN_MATCH) {
    return PreflateToken(PreflateToken::LITERAL);
  }
  unsigned hash = state.calculateHash();
  unsigned head = state.getCurrentHashHead(hash);
  PreflateToken match(PreflateToken::NONE);
  
  if (pendingToken.len > 1) {
    match = pendingToken;
  } else {
    match = state.match(head, prevLen, 0,
                        params.veryFarMatchesDetected,
                        params.matchesToStartDetected,
                        params.zlibCompatible ? 0 : (1 << params.log2OfMaxChainDepthM1));
  }
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  if (match.len < PreflateConstants::MIN_MATCH) {
    return PreflateToken(PreflateToken::LITERAL);
  }
  if (fast) {
    return match;
  }
  if (match.len == 3 && match.dist > TOO_FAR) {
    return PreflateToken(PreflateToken::LITERAL);
  }

  if (match.len < state.lazyMatchLength() && state.availableInputSize() >= (unsigned)match.len + 2) {
    unsigned hashNext = state.calculateHashNext();
    unsigned headNext = state.getCurrentHashHead(hashNext);
    PreflateToken matchNext = state.match(headNext, match.len, 1, 
                                          params.veryFarMatchesDetected,
                                          params.matchesToStartDetected,
                                          params.zlibCompatible ? 0 : (2 << params.log2OfMaxChainDepthM1));
    if (((hashNext ^ hash) & this->hash.hashMask) == 0) {
      unsigned maxSize = std::min(state.availableInputSize() - 1, (unsigned)PreflateConstants::MAX_MATCH);
      unsigned rle = 1;
      const unsigned char *c = state.inputCursor();
      unsigned char b = c[0];
      while (rle < maxSize && c[1 + rle] == b) {
        ++rle;
      }
      if (rle >= matchNext.len) {
        matchNext.len = rle;
        matchNext.dist = 1;
      }
    }
    if (matchNext.len > match.len) {
      prevLen = match.len;
      pendingToken = matchNext;
      if (!params.zlibCompatible) {
        prevLen = 0;
        pendingToken = PreflateToken(PreflateToken::NONE);
      }
      return PreflateToken(PreflateToken::LITERAL);
    }
  }

  return match;
}
bool PreflateTokenPredictor::repredictReference(PreflateToken& token) {
  if (state.currentInputPos() == 0 || state.availableInputSize() < PreflateConstants::MIN_MATCH) {
    return false;
  }
  unsigned hash = state.calculateHash();
  unsigned head = state.getCurrentHashHead(hash);
  PreflateToken match = state.match(head, /*prevLen*/0, 0, 
                                    params.veryFarMatchesDetected,
                                    params.matchesToStartDetected,
                                    (2 << params.log2OfMaxChainDepthM1));
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  if (match.len < PreflateConstants::MIN_MATCH) {
    return false;
  }
  token = match;
  return true;
}
PreflateRematchInfo PreflateTokenPredictor::repredictMatch(const PreflateToken& token) {
  unsigned hash = state.calculateHash();
  unsigned head = state.getCurrentHashHead(hash);
  PreflateRematchInfo i = state.rematchInfo(head, token);
  return i;
}
unsigned PreflateTokenPredictor::recalculateDistance(const PreflateToken& token, const unsigned hops) {
  return state.hopMatch(token, hops);
}

void PreflateTokenPredictor::analyzeBlock(
    const unsigned blockno, 
    const PreflateTokenBlock& block) {
  currentTokenCount = 0;
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  if (blockno != analysisResults.size() || predictionFailure) {
    return;
  }
  analysisResults.push_back(BlockAnalysisResult());
  BlockAnalysisResult& analysis = analysisResults[blockno];

  analysis.type = block.type;
  analysis.tokenCount = block.tokens.size();
  analysis.tokenInfo.resize(analysis.tokenCount);
  analysis.blockSizePredicted = true;
  analysis.inputEOF = false;

  if (analysis.type == PreflateTokenBlock::STORED) {
    analysis.tokenCount = block.uncompressedLen;
    hash.updateHash(block.uncompressedLen);
    analysis.inputEOF = state.availableInputSize() == 0;
    return;
  }

  for (unsigned i = 0, n = block.tokens.size(); i < n; ++i) {
    PreflateToken targetToken = block.tokens[i];
    if (targetToken.len > 258) {
      predictionFailure = true;
      return;
    }
    if (predictEOB()) {
      analysis.blockSizePredicted = false;
    }
    PreflateToken predictedToken = predictToken();

    if (targetToken.len == 1) {
      if (predictedToken.len > 1) {
        analysis.tokenInfo[currentTokenCount] = 2; // badly predicted LIT
      } else {
        analysis.tokenInfo[currentTokenCount] = 0; // perfectly predicted LIT
      }
    } else {
      if (predictedToken.len == 1) {
        analysis.tokenInfo[currentTokenCount] = 3; // badly predicted REF
        if (!repredictReference(predictedToken)) {
          predictionFailure = true;
          return;
        }
      } else {
        analysis.tokenInfo[currentTokenCount] = 1; // well predicted REF
      }
      PreflateRematchInfo rematch;
      if (predictedToken.len != targetToken.len) {
        analysis.tokenInfo[currentTokenCount] += 4; // bad LEN prediction, adds two corrective actions
        analysis.correctives.push_back(predictedToken.len);
        analysis.correctives.push_back(targetToken.len - predictedToken.len);
        rematch = repredictMatch(targetToken);

        if (rematch.requestedMatchDepth >= 0xffff) {
          predictionFailure = true;
          return;
        }
        analysis.correctives.push_back(rematch.condensedHops - 1);
      } else {
        if (targetToken.dist != predictedToken.dist) {
          analysis.tokenInfo[currentTokenCount] += 8; // bad DIST ONLY prediction, adds one corrective action
          rematch = repredictMatch(targetToken);

          if (rematch.requestedMatchDepth >= 0xffff) {
            predictionFailure = true;
            return;
          }
          analysis.correctives.push_back(rematch.condensedHops - 1);
        }
      }
    }
    commitToken(targetToken);
    ++currentTokenCount;
  }
  if (!predictEOB()) {
    analysis.blockSizePredicted = false;
  }
  analysis.inputEOF = state.availableInputSize() == 0;
}

void PreflateTokenPredictor::encodeBlock(
    PreflateStatisticalEncoder* codec,
    const unsigned blockno) {
  BlockAnalysisResult& analysis = analysisResults[blockno];

  codec->encode(CORR_BLOCK_TYPE, analysis.type);

  if (analysis.type == PreflateTokenBlock::STORED) {
    codec->encodeValue(analysis.tokenCount, 16);
    return;
  }

  codec->encode(CORR_EOB_MISPREDICTION, !analysis.blockSizePredicted);
  if (!analysis.blockSizePredicted) {
    unsigned blocksizeBits = bitLength(analysis.tokenCount);
    codec->encodeValue(blocksizeBits, 5);
    if (blocksizeBits >= 2) {
      codec->encodeValue(analysis.tokenCount, blocksizeBits);
    }
  }
 
  unsigned correctivePos = 0;
  for (unsigned i = 0, n = analysis.tokenCount; i < n; ++i) {
    unsigned char info = analysis.tokenInfo[i];
    switch (info & 3) {
    case 0: // well predicted LIT
      codec->encode(CORR_LIT_MISPREDICTION, 0);
      continue;
    case 2: // badly predicted LIT
      codec->encode(CORR_REF_MISPREDICTION, 1);
      continue;
    case 1: // well predicted REF
      codec->encode(CORR_REF_MISPREDICTION, 0);
      break;
    case 3: // badly predicted REF
      codec->encode(CORR_LIT_MISPREDICTION, 1);
      break;
    }
    codec->encode(CORR_LEN_MISPREDICTION, (info & 4) != 0);
    if (info & 4) {
      int pred = analysis.correctives[correctivePos++];
      int diff = analysis.correctives[correctivePos++];
      int hops = analysis.correctives[correctivePos++];
      codec->encode(CORR_LEN_CORRECTION, diff, pred);
      codec->encode(CORR_DIST_AFTER_LEN_CORRECTION, hops);
    } else {
      codec->encode(CORR_DIST_ONLY_MISPREDICTION, (info & 8) != 0);
      if (info & 8) {
        int hops = analysis.correctives[correctivePos++];
        codec->encode(CORR_DIST_ONLY_CORRECTION, hops);
      }
    }
  }
}
void PreflateTokenPredictor::encodeEOF(
    PreflateStatisticalEncoder* codec,
    const unsigned blockno,
    const bool lastBlock) {
  BlockAnalysisResult& analysis = analysisResults[blockno];

  if (analysis.inputEOF) {
    codec->encodeValue(!lastBlock, 1);
  } else {
    // If we still have input left, this shouldn't be the last block
    if (lastBlock) {
      predictionFailure = true;
    }
  }
}

void PreflateTokenPredictor::updateModel(
  PreflateStatisticalModel* model,
  const unsigned blockno) {
  BlockAnalysisResult& analysis = analysisResults[blockno];

  model->blockType[analysis.type]++;

  if (analysis.type == PreflateTokenBlock::STORED) {
    return;
  }

  model->EOBMisprediction[!analysis.blockSizePredicted]++;

  unsigned correctivePos = 0;
  for (unsigned i = 0, n = analysis.tokenCount; i < n; ++i) {
    unsigned char info = analysis.tokenInfo[i];
    switch (info & 3) {
    case 0: // well predicted LIT
      model->LITMisprediction[0]++;
      continue;
    case 2: // badly predicted LIT
      model->REFMisprediction[1]++;
      continue;
    case 1: // well predicted REF
      model->REFMisprediction[0]++;
      break;
    case 3: // badly predicted REF
      model->LITMisprediction[1]++;
      break;
    }
    model->LENMisprediction[(info & 4) != 0]++;
    if (info & 4) {
      /*int pred = analysis.correctives[*/correctivePos++/*]*/;
      int diff = analysis.correctives[correctivePos++];
      int hops = analysis.correctives[correctivePos++];
      if (diff > 0) {
        model->LENPositiveCorrection[std::min(5, diff - 1)]++;
      } else {
        model->LENNegativeCorrection[std::min(5, -diff - 1)]++;
      }
      model->DISTAfterLenCorrection[std::min(3, hops)]++;
    } else {
      model->DISTOnlyMisprediction[(info & 8) != 0]++;
      if (info & 8) {
        int hops = analysis.correctives[correctivePos++];
        model->DISTOnlyCorrection[std::min(3, hops)]++;
      }
    }
  }
}

PreflateTokenBlock PreflateTokenPredictor::decodeBlock(
    PreflateStatisticalDecoder* codec) {
  PreflateTokenBlock block;
  currentTokenCount = 0;
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  unsigned blocksize = 0;
  bool checkEOB = true;
  unsigned bt = codec->decode(CORR_BLOCK_TYPE);
  switch (bt) {
  case PreflateTokenBlock::STORED:
    block.type = PreflateTokenBlock::STORED;
    block.uncompressedLen = codec->decodeValue(16);
    hash.updateHash(block.uncompressedLen);
    return block;
  case PreflateTokenBlock::STATIC_HUFF:
    block.type = PreflateTokenBlock::STATIC_HUFF;
    break;
  case PreflateTokenBlock::DYNAMIC_HUFF:
    block.type = PreflateTokenBlock::DYNAMIC_HUFF;
    break;
  }

  if (codec->decode(CORR_EOB_MISPREDICTION)) {
    unsigned blocksizeBits = codec->decodeValue(5);
    if (blocksizeBits >= 2) {
      blocksize = codec->decodeValue(blocksizeBits);
    } else {
      blocksize = blocksizeBits;
    }
    block.tokens.reserve(blocksize);
    checkEOB = false;
  } else {
    block.tokens.reserve(1 << (6 + params.memLevel));
  }
  while ((checkEOB && !predictEOB())
         || (!checkEOB && currentTokenCount < blocksize)) {
    PreflateToken predictedToken = predictToken();
    if (predictedToken.len == 1) {
      unsigned notok = codec->decode(CORR_LIT_MISPREDICTION);
      if (!notok) {
        block.tokens.push_back(predictedToken);
        commitToken(predictedToken);
        ++currentTokenCount;
        continue;
      }
      if (!repredictReference(predictedToken)) {
        predictionFailure = true;
        return PreflateTokenBlock();
      }
    } else {
      unsigned notok = codec->decode(CORR_REF_MISPREDICTION);
      if (notok) {
        predictedToken.len = 1;
        predictedToken.dist = 0;
        block.tokens.push_back(predictedToken);
        commitToken(predictedToken);
        ++currentTokenCount;
        continue;
      }
    }
    bool lennotok = codec->decode(CORR_LEN_MISPREDICTION);
    if (lennotok) {
      int lendiff = codec->decode(CORR_LEN_CORRECTION, predictedToken.len);
      int hops    = codec->decode(CORR_DIST_AFTER_LEN_CORRECTION);
      predictedToken.len += lendiff;
      predictedToken.dist = state.firstMatch(predictedToken.len);
      if (hops) {
        predictedToken.dist = recalculateDistance(predictedToken, hops);
      }
      if (predictedToken.len < 3 || predictedToken.len > 258
          || predictedToken.dist == 0) {
        predictionFailure = true;
        return PreflateTokenBlock();
      }
    } else {
      bool distnotok = codec->decode(CORR_DIST_ONLY_MISPREDICTION);
      if (distnotok) {
        int hops = codec->decode(CORR_DIST_ONLY_CORRECTION);
        if (hops) {
          predictedToken.dist = recalculateDistance(predictedToken, hops);
        }
        if (predictedToken.dist == 0) {
          predictionFailure = true;
          return PreflateTokenBlock();
        }
      }
    }
    block.tokens.push_back(predictedToken);
    commitToken(predictedToken);
    ++currentTokenCount;
  }
  return block;
}
bool PreflateTokenPredictor::decodeEOF(PreflateStatisticalDecoder* codec) {
  if (state.availableInputSize() == 0) {
    return codec->decodeValue(1) == 0;
  }
  return false;
}
