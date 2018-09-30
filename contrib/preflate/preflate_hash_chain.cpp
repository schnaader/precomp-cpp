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
#include <string.h>
#include "preflate_constants.h"
#include "preflate_hash_chain.h"

PreflateHashChainExt::PreflateHashChainExt(
    const std::vector<unsigned char>& input_,
    const unsigned char memLevel)
  : _input(input_)
  , totalShift(-8) {
  hashBits = memLevel + 7;
  hashShift = (hashBits + PreflateConstants::MIN_MATCH - 1) / PreflateConstants::MIN_MATCH;
  hashMask = (1 << hashBits) - 1;
  head = new unsigned short[hashMask + 1];
  prev = new unsigned short[1 << 16];
  chainDepth = new unsigned[1 << 16];
  memset(head, 0, sizeof(short) * (hashMask + 1));
  memset(prev, 0, sizeof(short) * (1 << 16));
  memset(chainDepth, 0, sizeof(unsigned) * (1 << 16));
  runningHash = 0;
  if (_input.remaining() > 2) {
    updateRunningHash(_input.curChar(0));
    updateRunningHash(_input.curChar(1));
  }
}
PreflateHashChainExt::~PreflateHashChainExt() {
  delete[] head;
  delete[] chainDepth;
  delete[] prev;
}

void PreflateHashChainExt::updateHash(const unsigned l) {
  if (l > 0x180) {
    unsigned l_ = l;
    while (l_ > 0) {
      unsigned blk = std::min(l_, 0x180u);
      updateHash(blk);
      l_ -= blk;
    }
    return;
  }

  const unsigned char* b = _input.curChars();
  unsigned pos = _input.pos();
  if (pos - totalShift >= 0xfe08) {
    reshift();
  }
  for (unsigned i = 2; i < std::min(l + 2, _input.remaining()); ++i) {
    updateRunningHash(b[i]);
    unsigned h = runningHash & hashMask;
    unsigned p = (pos + i - 2) - totalShift;
    chainDepth[p] = chainDepth[head[h]] + 1;
    prev[p] = head[h];
    head[h] = p;
  }
  _input.advance(l);
}
void PreflateHashChainExt::skipHash(const unsigned l) {
  const unsigned char* b = _input.curChars();
  unsigned pos = _input.pos();
  if (pos - totalShift >= 0xfe08) {
    reshift();
  }
  unsigned remaining = _input.remaining();
  if (remaining > 2) {
    updateRunningHash(b[2]);
    unsigned h = runningHash & hashMask;
    unsigned p = (pos) - totalShift;
    chainDepth[p] = chainDepth[head[h]] + 1;
    prev[p] = head[h];
    head[h] = p;

    // Skipped data is not inserted into the hash chain,
    // but we must still update the chainDepth, to avoid
    // bad analysis results
    // --------------------
    for (unsigned i = 1; i < l; ++i) {
      unsigned p = (pos + i) - totalShift;
      chainDepth[p] = 0xffff8000;
    }
    // l must be at least 3
    if (remaining > l) {
      updateRunningHash(b[l]);
      if (remaining > l + 1) {
        updateRunningHash(b[l + 1]);
      }
    }
  }
  _input.advance(l);
}
void PreflateHashChainExt::reshift() {
  const unsigned short delta = 0x7e00;
  for (unsigned i = 0, n = hashMask + 1; i < n; ++i) {
    head[i] = std::max(head[i], delta) - delta;
  }
  for (unsigned i = delta + 8, n = 1 << 16; i < n; ++i) {
    prev[i - delta] = std::max(prev[i], delta) - delta;
  }
  memmove(chainDepth + 8, chainDepth + 8 + delta, (0x10000 - 8 - delta) * sizeof(chainDepth[0]));
  totalShift += delta;
}
