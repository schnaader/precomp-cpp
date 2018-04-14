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

#ifndef PREFLATE_HASH_CHAIN_H
#define PREFLATE_HASH_CHAIN_H

#include <algorithm>
#include "preflate_input.h"

struct PreflateHashIterator {
  const unsigned short* chain;
  const unsigned * chainDepth;
  const unsigned refPos;
  const unsigned maxDist;
  unsigned curPos, curDist;
  bool isValid;

  PreflateHashIterator(
      const unsigned short* chain_,
      const unsigned * depth_,
      const unsigned refPos_,
      const unsigned maxDist_,
      unsigned startPos_)
    : chain(chain_)
    , chainDepth(depth_)
    , refPos(refPos_)
    , maxDist(maxDist_)
    , curPos(startPos_)
    , curDist(dist(refPos_, startPos_)) {
    isValid = curDist <= maxDist;
  }

  inline bool valid() const {
    return isValid;
  }
  inline bool operator !() const {
    return !isValid;
  }
  static inline unsigned dist(const unsigned p1, const unsigned p2) {
    return p1 - p2;
  }
  inline unsigned dist() const {
    return curDist;
  }
  inline unsigned depth() const {
    return chainDepth[curPos];
  }
  inline bool next() {
    curPos = chain[curPos];
    curDist = dist(refPos, curPos);
    isValid = curPos > 0 && curDist <= maxDist;
    return isValid;
  }
};

struct PreflateHashChainExt {
  PreflateInput _input;
  unsigned short* head;
  unsigned * chainDepth;
  unsigned short* prev;
  unsigned char hashBits, hashShift;
  unsigned short runningHash, hashMask;
  unsigned totalShift;

  PreflateHashChainExt(const std::vector<unsigned char>& input_, const unsigned char memLevel);
  ~PreflateHashChainExt();

  unsigned nextHash(const unsigned char b) const {
    return ((runningHash << hashShift) ^ b);
  }
  unsigned nextHash(const unsigned char b1, const unsigned char b2) const {
    return ((((runningHash << hashShift) ^ b1) << hashShift) ^ b2);
  }
  void updateRunningHash(const unsigned char b) {
    runningHash = (runningHash << hashShift) ^ b;
  }
  void reshift();
  unsigned getHead(const unsigned hash) const {
    return head[hash & hashMask];
  }
  unsigned getNodeDepth(const unsigned node) const {
    return chainDepth[node];
  }

  PreflateHashIterator iterateFromHead(const unsigned hash, const unsigned refPos, const unsigned maxDist) const {
    return PreflateHashIterator(prev, chainDepth, refPos - totalShift, maxDist, head[hash & hashMask]);
  }
  PreflateHashIterator iterateFromNode(const unsigned node, const unsigned refPos, const unsigned maxDist) const {
    return PreflateHashIterator(prev, chainDepth, refPos - totalShift, maxDist, node);
  }
  PreflateHashIterator iterateFromPos(const unsigned pos, const unsigned refPos, const unsigned maxDist) const {
    return PreflateHashIterator(prev, chainDepth, refPos - totalShift, maxDist, pos - totalShift);
  }
  const PreflateInput& input() const {
    return _input;
  }
  unsigned curHash() const {
    return nextHash(_input.curChar(2));
  }
  unsigned curPlus1Hash() const {
    return nextHash(_input.curChar(2), _input.curChar(3));
  }
  void updateHash(const unsigned l);
  void updateHashLong(const unsigned l);
  void skipHash(const unsigned l);

private:
  void _updateHashSimple(const unsigned l);
};

#endif /* PREFLATE_HASH_CHAIN_H */
