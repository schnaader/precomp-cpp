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

#ifndef PREFLATE_SEQ_CHAIN_H
#define PREFLATE_SEQ_CHAIN_H

#include <algorithm>
#include "preflate_input.h"

struct SeqChainEntry {
  uint16_t distToNext;
  uint16_t length;
};

struct PreflateSeqIterator {
  const SeqChainEntry* chain;
  const unsigned refPos;
  unsigned curDist;

  PreflateSeqIterator(
      const SeqChainEntry* chain_,
      const unsigned refPos_)
    : chain(chain_)
    , refPos(refPos_)
    , curDist(chain_[refPos_].distToNext) {
  }

  inline bool valid() const {
    return curDist <= refPos - 8;
  }
  inline bool operator !() const {
    return !valid();
  }
  inline unsigned dist() const {
    return curDist;
  }
  inline uint16_t len() const {
    return chain[refPos - curDist].length;
  }
  inline bool next() {
    curDist += chain[refPos - curDist].distToNext;
    return valid();
  }
};

struct PreflateSeqChain {
  PreflateInput _input;
  SeqChainEntry* prev;
  unsigned totalShift;
  unsigned curPos;
  uint16_t heads[256];

  PreflateSeqChain(const std::vector<unsigned char>& input_);
  ~PreflateSeqChain();

  bool valid(const unsigned refPos) const {
    return prev[refPos - totalShift].distToNext != 0xffff;
  }
  uint16_t len(const unsigned refPos) const {
    return prev[refPos - totalShift].length;
  }
  PreflateSeqIterator iterateFromPos(const unsigned refPos) const {
    return PreflateSeqIterator(prev, refPos - totalShift);
  }
  void updateSeq(const unsigned l) {
    curPos += l;
    while (curPos - totalShift >= 0xfe08) {
      _reshift();
    }
  }

private:
  void _reshift();
  void _build(const unsigned off0, const unsigned size);
};

#endif /* PREFLATE_SEQ_CHAIN_H */
