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

#include "preflate_constants.h"
#include "preflate_predictor_state.h"
#include <algorithm>

PreflatePredictorState::PreflatePredictorState(
    const PreflateHashChainExt& hash_,
    const PreflateSeqChain& seq_,
    const PreflateParserConfig& config_,
    const int wbits, 
    const int mbits) 
  : hash(hash_)
  , seq(seq_)
  , windowBytes(1 << wbits)
  , maxTokenCount((1 << (6 + mbits)) - 1)
  , config(config_) {
}

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

unsigned PreflatePredictorState::prefixCompare(
    const unsigned char* s1, 
    const unsigned char* s2, 
    const unsigned bestLen,
    const unsigned maxLen) {
  if (s1[bestLen] != s2[bestLen]) {
    return 0;
  }
  if (s1[0] != s2[0] || s1[1] != s2[1] || s1[2] != s2[2]) {
    return 0;
  }

  const unsigned char* scan  = s2 + 3;
  const unsigned char* match = s1 + 3;
  const unsigned char* scanend = s2 + maxLen;

/* while (scan < scanend
          && *++scan == *++match && *++scan == *++match
          && *++scan == *++match && *++scan == *++match
          && *++scan == *++match && *++scan == *++match
          && *++scan == *++match && *++scan == *++match) {
 }*/
  while (scan < scanend
          && *scan == *match) {
    ++scan;
    ++match;
  }

  return scan - s2;
}

unsigned PreflatePredictorState::suffixCompare(
    const unsigned char* s1,
    const unsigned char* s2,
    const unsigned bestLen,
    const unsigned maxLen) {
  if (s1[bestLen] != s2[bestLen]) {
    return 0;
  }
  unsigned len = 0;
  while (s1[len] == s2[len] && ++len < maxLen) {
  }
  return len;
}

bool PreflatePredictorState::createMatchHelper(
    MatchHelper& helper,
    const unsigned prevLen,
    const unsigned startPos,
    const bool veryFarMatches,
    const bool matchesToStart,
    const unsigned maxDepth) {
  helper.maxLen = std::min(totalInputSize() - startPos, (unsigned)PreflateConstants::MAX_MATCH);
  if (helper.maxLen < std::max<uint32_t>(prevLen + 1, PreflateConstants::MIN_MATCH)) {
    return false;
  }
  helper.startPos = startPos;
  unsigned maxDistToStart = startPos - (matchesToStart ? 0 : 1);
  if (veryFarMatches) {
    helper.curMaxDistHop1Plus
      = helper.curMaxDistHop0
      = std::min(maxDistToStart, windowSize());
  } else {
    unsigned maxDist = windowSize() - PreflateConstants::MIN_LOOKAHEAD;
    helper.curMaxDistHop0 = std::min(maxDistToStart, maxDist);
    helper.curMaxDistHop1Plus = std::min(maxDistToStart, maxDist - 1);
  }
  if (maxDepth > 0) {
    helper.maxChain = maxDepth;
    helper.niceLen = helper.maxLen;
  } else {
    helper.maxChain = maxChainLength();/* max hash chain length */
    helper.niceLen = std::min(niceMatchLength(), helper.maxLen);

    if (prevLen >= goodMatchLength()) {
      helper.maxChain >>= 2;
    }
  }
  return true;
}

PreflateToken PreflatePredictorState::match(
    const unsigned hashHead,
    const unsigned prevLen,
    const unsigned offset,
    const bool veryFarMatches,
    const bool matchesToStart,
    const unsigned maxDepth) {
  PreflateToken bestMatch(PreflateToken::NONE);
  MatchHelper h;
  if (!createMatchHelper(h, prevLen, currentInputPos() + offset,
                         veryFarMatches, matchesToStart, maxDepth)) {
    return bestMatch;
  }
  PreflateHashIterator chainIt = iterateFromNode(hashHead, h.startPos, h.curMaxDistHop1Plus);
  // Handle ZLIB quirk: the very first entry in the hash chain can have a larger
  // distance than all following entries
  if (chainIt.dist() > h.curMaxDistHop0) {
    return bestMatch;
  }
  const unsigned char* input = inputCursor() + offset;
  unsigned bestLen = prevLen;
  do {
    const unsigned char* match = input - chainIt.dist();

    unsigned matchLength = prefixCompare(match, input, bestLen, h.maxLen);
    if (matchLength > bestLen) {
      bestLen = matchLength;
      bestMatch = PreflateToken(PreflateToken::REFERENCE, matchLength, chainIt.dist());
      if (bestLen >= h.niceLen) {
        break;
      }
    }
  } while (chainIt.next() && h.maxChain-- > 1);
  return bestMatch;
}
PreflateToken PreflatePredictorState::seqMatch(
  const unsigned startPos,
  const unsigned hashHead,
  const unsigned prevLen,
  const bool veryFarMatches,
  const bool matchesToStart,
  const unsigned maxDepth) {
  PreflateToken bestMatch(PreflateToken::NONE);
  MatchHelper h;
  if (!createMatchHelper(h, prevLen, startPos,
                         veryFarMatches, matchesToStart, maxDepth)) {
    return bestMatch;
  }

  PreflateSeqIterator chainIt = seq.iterateFromPos(startPos);
  if (!chainIt) {
    return bestMatch;
  }
  unsigned curSeqLen = std::min<uint32_t>(seq.len(startPos), h.maxLen);
  unsigned curMaxDist = h.curMaxDistHop1Plus;
  unsigned bestLen = prevLen;
  if (curSeqLen < PreflateConstants::MIN_MATCH) {
    // startPos is part of a bigger sequence,
    // and the ZLIB quirk does not apply, yeah!
    curSeqLen = std::min(chainIt.len() - chainIt.dist(), h.maxLen);
    if (curSeqLen > prevLen && 1 <= h.curMaxDistHop0) {
      bestLen = curSeqLen;
      bestMatch = PreflateToken(PreflateToken::REFERENCE, curSeqLen, 1);
    }
    if (bestLen >= h.niceLen || !chainIt.next()) {
      return bestMatch;
    }
    if (chainIt.dist() > h.curMaxDistHop1Plus + chainIt.len() - PreflateConstants::MIN_MATCH) {
      return bestMatch;
    }
  } else {
    unsigned minDistOff = chainIt.len() - PreflateConstants::MIN_MATCH;
    if (chainIt.dist() > h.curMaxDistHop1Plus + minDistOff) {
      if (chainIt.dist() > h.curMaxDistHop0 + minDistOff) {
        return bestMatch;
      }
      // Handle ZLIB quirk: the very first entry in the hash chain can have a larger
      // distance than all following entries
      unsigned latestPos = h.startPos - chainIt.dist() + minDistOff;
      unsigned depth = hash.getRelPosDepth(latestPos, hashHead);
      if (depth == 0) {
        curMaxDist = h.curMaxDistHop0;
      }
    }
  }
  const unsigned char* input = inputCursor() + startPos - currentInputPos();
  unsigned bestSeqLen = std::min(curSeqLen, bestLen);

  do {
    if (chainIt.len() < bestSeqLen) {
      // If we do not even meet the already matched number of sequence bytes,
      // we can just skip this
      continue;
    }

    unsigned oldBestSeqLen = bestSeqLen;
    bestSeqLen = std::min<uint32_t>(std::min<uint32_t>(curSeqLen, chainIt.len()), h.niceLen);
    unsigned bestDist = chainIt.dist() - chainIt.len() + bestSeqLen;
    unsigned error = 0;
    if (bestDist > curMaxDist) {
      // best subsequence is already beyond the search range
      error = bestDist - curMaxDist;
      if (error > chainIt.len() - PreflateConstants::MIN_MATCH) {
        break;
      }
    }
    unsigned bestChainDepth = hash.getRelPosDepth(h.startPos - bestDist + error, hashHead);
    if (bestChainDepth >= h.maxChain) {
      // best subsequence is already beyond the search range
      error += bestChainDepth - h.maxChain + 1;
      if (error > chainIt.len() - PreflateConstants::MIN_MATCH) {
        break;
      }
    }
    if (error) {
      if (bestSeqLen > std::max<uint32_t>(oldBestSeqLen, PreflateConstants::MIN_MATCH - 1) + error) {
        bestMatch = PreflateToken(PreflateToken::REFERENCE, bestSeqLen - error, bestDist - error);
      }
      // Since we had to correct the length down, we know that
      // the comparer cannot find a better match
      break;
    }
    if (bestSeqLen == h.maxLen) {
      bestMatch = PreflateToken(PreflateToken::REFERENCE, bestSeqLen, bestDist);
      break;
    } else {
      const unsigned char* match = input - bestDist;

      unsigned matchLength = bestSeqLen + suffixCompare(match + bestSeqLen, input + bestSeqLen, std::max(bestLen, bestSeqLen) - bestSeqLen, h.maxLen - bestSeqLen);
      if (matchLength > bestLen) {
        bestLen = matchLength;
        bestMatch = PreflateToken(PreflateToken::REFERENCE, matchLength, bestDist);
        if (bestLen >= h.niceLen) {
          break;
        }
      }
    }
    curMaxDist = h.curMaxDistHop1Plus;
  } while (chainIt.next());
  return bestMatch;
}

PreflateNextMatchInfo PreflatePredictorState::nextMatchInfo(
  const unsigned hashHead,
  const PreflateToken& targetReference,
  const PreflateHashChainExt& hash) {
  PreflateNextMatchInfo result;
  result.nextChainDepth = (unsigned short)~0u;
  result.nextLen = 0;
  result.nextDist = 0xffff;
  unsigned maxLen = std::min(availableInputSize(), (unsigned)PreflateConstants::MAX_MATCH);
  if (maxLen < (unsigned)PreflateConstants::MIN_MATCH) {
    return result;
  }

  unsigned maxDist = windowSize() - PreflateConstants::MIN_LOOKAHEAD - 1;
  unsigned curPos = currentInputPos();
  unsigned curMaxDist = std::min(curPos - 1, maxDist);
  unsigned curMaxDistAlt = std::min(curPos - 1, windowSize() - PreflateConstants::MIN_LOOKAHEAD);

  const unsigned char* input = inputCursor();
  unsigned startDepth = hash.getNodeDepth(hashHead);
  unsigned maxChainOrg = maxChainLength();/* max hash chain length */
  PreflateHashIterator chainIt = hash.iterateFromPos(curPos - targetReference.dist, curPos, curMaxDist);
  if (!chainIt.curPos || (hashHead == chainIt.curPos && chainIt.dist() > curMaxDistAlt)
      || (hashHead != chainIt.curPos  && chainIt.dist() > curMaxDist)) {
    return result;
  }
  unsigned endDepth = chainIt.depth();
  unsigned maxChain = maxChainOrg - std::min(startDepth - endDepth, 0xffffu);/* max hash chain length */

  unsigned bestLen = targetReference.len;

  while (maxChain > 0) {
    if (!chainIt.next()) {
      break;
    }
    const unsigned char* match = input - chainIt.dist();

    unsigned matchLength = prefixCompare(match, input, bestLen, maxLen);
    if (matchLength > bestLen) {
      result.nextLen = matchLength;
      result.nextChainDepth = maxChainOrg - maxChain;
      result.nextDist = chainIt.dist();
      break;
    }
    --maxChain;
  }
  return result;
}

PreflateRematchInfo PreflatePredictorState::rematchInfo(
    const unsigned hashHead,
    const PreflateToken& targetReference) {
  PreflateRematchInfo result;
  result.firstMatchDepth = 0xffff;
  result.requestedMatchDepth = 0xffff;
  result.condensedHops = 0;
  unsigned maxLen = std::min(availableInputSize(), (unsigned)PreflateConstants::MAX_MATCH);
  if (maxLen < targetReference.len) {
    return result;
  }

  unsigned maxDist = windowSize();
  unsigned curPos = currentInputPos();
  unsigned curMaxDist = std::min(curPos, maxDist);

  PreflateHashIterator chainIt = hash.iterateFromNode(hashHead, curPos, curMaxDist);
  if (!chainIt) {
    return result;
  }
  const unsigned char* input = inputCursor();

  unsigned maxChainOrg = 0xffff;/* max hash chain length */
  unsigned maxChain = maxChainOrg;/* max hash chain length */

  unsigned bestLen = targetReference.len;

  do {
    const unsigned char* match = input - chainIt.dist();

    unsigned matchLength = prefixCompare(match, input, bestLen - 1, bestLen);
    if (matchLength >= bestLen) {
      result.firstMatchDepth = std::min((unsigned)result.firstMatchDepth, maxChainOrg - maxChain);
      result.condensedHops++;
    }
    if (chainIt.dist() >= targetReference.dist) {
      if (chainIt.dist() == targetReference.dist) {
        result.requestedMatchDepth = maxChainOrg - maxChain;
      }
      return result;
    }

    chainIt.next();
  } while (!!chainIt && maxChain-- > 1);
  return result;
}
unsigned PreflatePredictorState::firstMatch(const unsigned len) {
  unsigned maxLen = std::min(availableInputSize(), (unsigned)PreflateConstants::MAX_MATCH);
  if (maxLen < std::max(len, (unsigned)PreflateConstants::MIN_MATCH)) {
    return 0;
  }

  unsigned curPos = currentInputPos();
  unsigned curMaxDist = std::min(curPos, windowSize());

  unsigned hash = calculateHash();

  PreflateHashIterator chainIt = iterateFromHead(hash, curPos, curMaxDist);
  if (!chainIt) {
    return 0;
  }
  const unsigned char* input = inputCursor();

  do {
    const unsigned char* match = input - chainIt.dist();

    unsigned matchLength = prefixCompare(match, input, len - 1, len);
    if (matchLength >= len) {
      return chainIt.dist();
    }
  } while (chainIt.next());
  return 0;
}

unsigned PreflatePredictorState::hopMatch(const PreflateToken& targetReference, const unsigned hops) {
  if (hops == 0) {
    return targetReference.dist;
  }

  unsigned curPos   = currentInputPos();
  unsigned errorDist = 0;
  unsigned maxLen = std::min(availableInputSize(), (unsigned)PreflateConstants::MAX_MATCH);
  if (maxLen < targetReference.len) {
    return errorDist;
  }
  unsigned maxDist = windowSize();
  unsigned curMaxDist = std::min(curPos, maxDist);

  PreflateHashIterator chainIt = iterateFromDist(targetReference.dist, curPos, curMaxDist);
  if (!chainIt) {
    return 0;
  }

  const unsigned char* input = inputCursor();

  unsigned bestLen = targetReference.len;
  for (unsigned todo = hops; todo > 0; ) {
    if (!chainIt.next()) {
      break;
    }

    const unsigned char* match = input - chainIt.dist();

    unsigned matchLength = prefixCompare(match, input - targetReference.dist, bestLen - 1, bestLen);
    if (matchLength >= bestLen) {
      if (--todo == 0) {
        return chainIt.dist();
      }
    }
  }
  return errorDist;
}
