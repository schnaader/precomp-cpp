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

#ifndef PREFLATE_PREDICTOR_STATE_H
#define PREFLATE_PREDICTOR_STATE_H

#include <vector>

#include "preflate_input.h"
#include "preflate_hash_chain.h"
#include "preflate_parser_config.h"
#include "preflate_token.h"

struct PreflatePreviousMatchInfo {
  PreflateToken previousMatches[256];
};

struct PreflateNextMatchInfo {
  unsigned short nextChainDepth;
  unsigned short nextLen;
  unsigned short nextDist;
};

struct PreflateRematchInfo {
  unsigned short firstMatchDepth;
  unsigned short firstMatchDist;
  unsigned short requestedMatchDepth;
  unsigned short condensedHops;
};

struct PreflatePredictorState {
  const PreflateHashChainExt&    hash;
  unsigned short windowBytes;
  unsigned maxTokenCount;
  const PreflateParserConfig& config;

  PreflatePredictorState(const PreflateHashChainExt&,
                         const PreflateParserConfig&,
                         const int wbits, 
                         const int mbits);

  unsigned currentInputPos() const {
    return hash.input().pos();
  }
  const unsigned char* inputCursor() const {
    return hash.input().curChars();
  }
  unsigned windowSize() const {
    return windowBytes;
  }
  unsigned availableInputSize() const {
    return hash.input().remaining();
  }
  unsigned maxChainLength() const {
    return config.max_chain;
  }
  unsigned niceMatchLength() const {
    return config.nice_length;
  }
  unsigned goodMatchLength() const {
    return config.good_length;
  }
  unsigned lazyMatchLength() const {
    return config.max_lazy;
  }
  unsigned calculateHash() const {
    return hash.curHash();
  }
  unsigned calculateHashNext() const {
    return hash.curPlus1Hash();
  }
  unsigned getCurrentHashHead(const unsigned hashNext) const {
    return hash.getHead(hashNext);
  }

  PreflateHashIterator iterateFromHead(const unsigned hash_, const unsigned refPos, const unsigned maxDist) const {
    return hash.iterateFromHead(hash_, refPos, maxDist);
  }
  PreflateHashIterator iterateFromNode(const unsigned node_, const unsigned refPos, const unsigned maxDist) const {
    return hash.iterateFromNode(node_, refPos, maxDist);
  }
  PreflateHashIterator iterateFromDist(const unsigned dist_, const unsigned refPos, const unsigned maxDist) const {
    return hash.iterateFromPos(refPos - dist_, refPos, maxDist);
  }

  static unsigned prefixCompare(
      const unsigned char* s1,
      const unsigned char* s2,
      const unsigned bestLen,
      const unsigned maxLen);

  PreflateToken match(
      const unsigned hashHead, 
      const unsigned prevLen, 
      const unsigned offset, 
      const bool veryFarMatches,
      const bool matchesToStart,
      const unsigned maxDepth);
  unsigned short matchDepth(const unsigned hashHead, const PreflateToken& targetReference,
                      const PreflateHashChainExt&);
  PreflateNextMatchInfo nextMatchInfo(const unsigned hashHead, const PreflateToken& targetReference,
                              const PreflateHashChainExt&);
  PreflateRematchInfo rematchInfo(const unsigned hashHead, const PreflateToken& targetReference);
  unsigned firstMatch(const unsigned len);
  unsigned hopMatch(const PreflateToken& token, const unsigned hops);
};

#endif /* PREFLATE_PREDICTOR_STATE_H */
